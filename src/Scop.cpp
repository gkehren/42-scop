#include "../include/Scop.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "../imgui/stb_image.h"

const char* vertexShaderSource = R"(
	#version 330 core

	layout(location = 0) in vec3 inPosition;
	layout(location = 1) in vec2 inTexCoord;
	layout(location = 2) in vec3 inNormal;

	out vec2 TexCoord;
	out vec3 FragPos;
	out vec3 Normal;

	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

	void main() {
		TexCoord = inTexCoord;
		FragPos = vec3(model * vec4(inPosition, 1.0));
		Normal = mat3(transpose(inverse(model))) * inNormal;

		gl_Position = projection * view * model * vec4(inPosition, 1.0);
	}
)";

const char* fragmentShaderSource = R"(
	#version 330 core

	in vec2 TexCoord;
	in vec3 FragPos;
	in vec3 Normal;

	out vec4 FragColor;

	uniform vec3 lightPos;
	uniform vec3 viewPos;
	uniform vec3 lightColor;
	uniform vec3 objectColor;

	uniform sampler2D textureSampler; // Texture sampler
	uniform bool showTextures;
	uniform bool showGradient;
	uniform bool showLight;

	void main() {
		vec3 result = vec3(0.0);

		if (showTextures) {
			vec3 textureColor = texture(textureSampler, TexCoord).xyz;
			result = textureColor;
		} else if (showGradient) {
			vec3 gradientColor = mix(vec3(TexCoord.y, TexCoord.x, 0.5), objectColor, 0.5);
			result = gradientColor;
		} else {
			result = objectColor;
		}

		if (showLight) {
			// Calculate lighting
			vec3 ambient = 0.1 * lightColor;
			vec3 norm = normalize(Normal);
			vec3 lightDir = normalize(lightPos - FragPos);
			float diff = max(dot(norm, lightDir), 0.0);
			vec3 diffuse = diff * lightColor;

			vec3 viewDir = normalize(viewPos - FragPos);
			vec3 reflectDir = reflect(-lightDir, norm);
			float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
			vec3 specular = spec * lightColor;

			result = result * (ambient + diffuse + specular);
		}

		FragColor = vec4(result, 1.0);
	}
)";

void	errorCallback(int error, const char* description)
{
	(void)error;
	std::cerr << "Erreur GLFW : " << description << std::endl;
}

void	Scop::scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	Scop* scop = static_cast<Scop*>(glfwGetWindowUserPointer(window));
	scop->processMouseScroll(yoffset);
}

Scop::Scop()
{
	// Initialize GLFW and OpenGL
	if (!glfwInit())
	{
		std::cerr << "Error while initializing GLFW" << std::endl;
		return;
	}

	glfwSetErrorCallback(errorCallback);

	// Use OpenGL 3.3 Core Profile
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

	// Create window
	this->windowWidth = 1920;
	this->windowHeight = 1080;

	this->window = glfwCreateWindow(this->windowWidth, this->windowHeight, "scop", nullptr, nullptr);
	if (!this->window)
	{
		std::cerr << "Error while creating GLFW window" << std::endl;
		glfwTerminate();
		return;
	}

	glfwMakeContextCurrent(this->window);

	// Initialize GLEW
	if (glewInit() != GLEW_OK)
	{
		std::cerr << "Error while initializing GLEW" << std::endl;
		glfwTerminate();
		return;
	}

	// Initialize ImGui
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(this->window, true);
	ImGui_ImplOpenGL3_Init();

	glEnable(GL_DEPTH_TEST);

	this->deltaTime = 0.0f;
	this->lastFrame = 0.0f;
	this->totalTime = 0.0f;
	this->updateInterval = 0.1f;
	this->frames = 0;

	glfwSetWindowUserPointer(this->window, this);
	glfwSetScrollCallback(this->window, scrollCallback);
	glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	this->lastX = this->windowWidth / 2.0f;
	this->lastY = this->windowHeight / 2.0f;
	this->firstMouse = true;
	this->yaw = -90.0f;
	this->pitch = 0.0f;
	this->distanceFromCube = 8.0f;
	this->sensitivity = 0.005f;
	this->fps = 0.0f;
	this->showTextures = false;
	this->showWireframe = false;
	this->showGradient = false;
	this->showLight = true;

	this->cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
	this->cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	this->cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
	this->cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	this->view = glm::lookAt(this->cameraPos, this->cameraTarget, this->cameraUp);
	this->projection = glm::perspective(glm::radians(45.0f), (float)this->windowWidth / (float)this->windowHeight, 0.1f, 100.0f);
	this->model = glm::mat4(1.0f);

	this->lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
	this->lightPos = glm::vec3(0.0f, 10.0f, 0.0f);
	this->objectColor = glm::vec3(1.0f, 0.5f, 0.31f);

	this->movementSpeed = 0.05f;
	this->rotationSpeed = 0.5f;
	this->rotationAngle = 0.0f;
	this->objectPosition = glm::vec3(0.0f, 0.0f, 0.0f);

	this->loadObjFile("/home/gkehren/42-scop/ressources/42.obj");
	this->loadbmpFile("/home/gkehren/42-scop/ressources/chaton.bmp");
}

Scop::~Scop()
{
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(this->window);
	glfwTerminate();
}

void	Scop::run()
{
	//glm::vec3 initialModelPosition  = calculateModelCenterOffset();

	while (!glfwWindowShouldClose(this->window))
	{
		glfwPollEvents();

		float currentFrame = glfwGetTime();
		this->deltaTime = currentFrame - this->lastFrame;
		this->lastFrame = currentFrame;
		this->totalTime += this->deltaTime;
		this->frames++;

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		this->cameraMovement();
		this->objectMovement();

		//glm::vec3 currentModelPosition = glm::vec3(this->model[3]);

		//this->objectPosition = initialModelPosition;
		//this->model = glm::rotate(this->model, rotationSpeed * deltaTime, glm::vec3(0.0f, 1.0f, 0.0f));
		//this->objectPosition = currentModelPosition;

		//this->model = glm::translate(glm::mat4(1.0f), initialModelPosition);
		//this->model = glm::rotate(this->model, rotationSpeed * deltaTime, glm::vec3(0.0f, 1.0f, 0.0f));
		//this->model[3] = glm::vec4(currentModelPosition, 1.0f);

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(this->shaderProgram);
		glUniformMatrix4fv(glGetUniformLocation(this->shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(this->model));
		glUniformMatrix4fv(glGetUniformLocation(this->shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(this->view));
		glUniformMatrix4fv(glGetUniformLocation(this->shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(this->projection));
		glUniform3fv(glGetUniformLocation(this->shaderProgram, "lightColor"), 1, glm::value_ptr(this->lightColor));
		glUniform3fv(glGetUniformLocation(this->shaderProgram, "lightPos"), 1, glm::value_ptr(this->lightPos));
		glUniform3fv(glGetUniformLocation(this->shaderProgram, "objectColor"), 1, glm::value_ptr(this->objectColor));
		glUniform1i(glGetUniformLocation(this->shaderProgram, "textureSampler"), 0);
		glUniform1i(glGetUniformLocation(this->shaderProgram, "showTextures"), this->showTextures);
		glUniform1i(glGetUniformLocation(this->shaderProgram, "showGradient"), this->showGradient);
		glUniform1i(glGetUniformLocation(this->shaderProgram, "showLight"), this->showLight);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureID);

		if (showWireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glBindVertexArray(this->VAO);
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, this->normalVBO);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		glDrawElements(GL_TRIANGLES, this->vertexIndices.size(), GL_UNSIGNED_INT, 0);
		glDisableVertexAttribArray(1);
		glBindVertexArray(0);

		this->updateUI();

		glfwSwapBuffers(window);
	}
}

void	Scop::processMouseScroll(double yoffset)
{
	if (this->distanceFromCube >= 1.0f && this->distanceFromCube <= 45.0f)
		this->distanceFromCube -= static_cast<float>(yoffset);
	if (this->distanceFromCube < 1.0f)
		this->distanceFromCube = 1.0f;
	if (this->distanceFromCube > 45.0f)
		this->distanceFromCube = 45.0f;
}

void	Scop::cameraMovement()
{
	double xPos, yPos;
	glfwGetCursorPos(this->window, &xPos, &yPos);

	if (this->firstMouse)
	{
		this->lastX = xPos;
		this->lastY = yPos;
		this->firstMouse = false;
	}

	float xOffset = xPos - this->lastX;
	float yOffset = this->lastY - yPos;
	xOffset *= this->sensitivity;
	yOffset *= this->sensitivity;

	if (glfwGetMouseButton(this->window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE)
	{
		xOffset = 0.0f;
		yOffset = 0.0f;
		this->lastX = xPos;
		this->lastY = yPos;
	}

	this->yaw += xOffset;
	this->pitch += yOffset;

	if (this->pitch > 89.0f)
		this->pitch = 89.0f;

	if (this->pitch < -89.0f)
		this->pitch = -89.0f;

	glm::vec3 front;
	front.x = cos(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));
	front.y = sin(glm::radians(this->pitch));
	front.z = sin(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));
	this->cameraFront = glm::normalize(front);

	this->cameraPos = this->cameraTarget - this->distanceFromCube * this->cameraFront;

	this->view = glm::lookAt(this->cameraPos, this->cameraTarget, this->cameraUp);
}

void Scop::objectMovement()
{
	glm::mat3 rotationMatrix3 = glm::mat3(this->model);
	glm::mat3 invRotationMatrix = glm::transpose(rotationMatrix3);

	glm::vec3 translationDelta(0.0f);

	if (glfwGetKey(this->window, GLFW_KEY_W) == GLFW_PRESS)
		translationDelta += movementSpeed * glm::normalize(invRotationMatrix * glm::vec3(0.0f, 0.0f, -1.0f));
	if (glfwGetKey(this->window, GLFW_KEY_S) == GLFW_PRESS)
		translationDelta -= movementSpeed * glm::normalize(invRotationMatrix * glm::vec3(0.0f, 0.0f, -1.0f));
	if (glfwGetKey(this->window, GLFW_KEY_A) == GLFW_PRESS)
		translationDelta -= movementSpeed * glm::normalize(invRotationMatrix * glm::vec3(1.0f, 0.0f, 0.0f));
	if (glfwGetKey(this->window, GLFW_KEY_D) == GLFW_PRESS)
		translationDelta += movementSpeed * glm::normalize(invRotationMatrix * glm::vec3(1.0f, 0.0f, 0.0f));
	if (glfwGetKey(this->window, GLFW_KEY_Q) == GLFW_PRESS)
		translationDelta += movementSpeed * glm::vec3(0.0f, 1.0f, 0.0f);
	if (glfwGetKey(this->window, GLFW_KEY_E) == GLFW_PRESS)
		translationDelta -= movementSpeed * glm::vec3(0.0f, 1.0f, 0.0f);
	if (glfwGetKey(this->window, GLFW_KEY_SPACE) == GLFW_PRESS)
		translationDelta = -objectPosition;

	this->objectPosition += translationDelta;
	this->rotationAngle = this->rotationSpeed * this->deltaTime;

	glm::mat4 rotationMatrix4 = glm::rotate(glm::mat4(1.0f), this->rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::vec3 modelCenterOffset = calculateModelCenterOffset();

	this->model = glm::translate(glm::mat4(1.0f), this->objectPosition - modelCenterOffset) * glm::mat4(rotationMatrix3) * rotationMatrix4 * glm::translate(glm::mat4(1.0f), modelCenterOffset);
}

void	Scop::updateUI()
{
	if (this->totalTime >= this->updateInterval)
	{
		this->fps = static_cast<float>(this->frames) / this->totalTime;
		this->frames = 0;
		this->totalTime = 0.0f;
	}
	ImGui::Begin("scop");
	ImGui::Text("FPS : %.1f", this->fps);
	ImGui::Text("Model position : (%.1f, %.1f, %.1f)", this->objectPosition.x, this->objectPosition.y, this->objectPosition.z);
	ImGui::Text("Camera position : (%.1f, %.1f, %.1f)", this->cameraPos.x, this->cameraPos.y, this->cameraPos.z);
	ImGui::Text("Camera front : (%.1f, %.1f, %.1f)", this->cameraFront.x, this->cameraFront.y, this->cameraFront.z);
	ImGui::Text("Camera target : (%.1f, %.1f, %.1f)", this->cameraTarget.x, this->cameraTarget.y, this->cameraTarget.z);
	ImGui::Text("Camera up : (%.1f, %.1f, %.1f)", this->cameraUp.x, this->cameraUp.y, this->cameraUp.z);
	ImGui::Text("Yaw : %.1f", this->yaw);
	ImGui::Text("Pitch : %.1f", this->pitch);
	// File Dialog
	if (ImGui::Button("Open File"))
		ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".obj", ".");

	if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey"))
	{
		if (ImGuiFileDialog::Instance()->IsOk())
		{
			std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
			this->loadObjFile(filePathName);
		}
		ImGuiFileDialog::Instance()->Close();
	}
	if (ImGui::Button("Reset object"))
		this->objectPosition = glm::vec3(0.0f, 0.0f, 0.0f);
	if (ImGui::Button("Reset camera"))
	{
		this->yaw = -90.0f;
		this->pitch = 0.0f;
		this->distanceFromCube = 8.0f;
	}
	ImGui::SliderFloat("Rotation Speed", &this->rotationSpeed, 0.0f, 2.0f);
	ImGui::Checkbox("Wireframe", &this->showWireframe);
	ImGui::Checkbox("Texture", &this->showTextures);
	ImGui::Checkbox("Gradient", &this->showGradient);
	ImGui::Checkbox("Light", &this->showLight);
	ImGui::ColorEdit3("Object Color", glm::value_ptr(this->objectColor));
	ImGui::ColorEdit3("Light Color", glm::value_ptr(this->lightColor));

	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void	Scop::loadbmpFile(std::string filePathName)
{
	this->imageData = stbi_load(filePathName.c_str(), &this->imageWidth, &this->imageHeight, &this->imageChannels, 0);
	if (!this->imageData)
	{
		std::cout << "Error: could not load texture" << std::endl;
		return ;
	}

	glGenTextures(1, &this->textureID);
	glBindTexture(GL_TEXTURE_2D, this->textureID);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, this->imageWidth, this->imageHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, this->imageData);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	stbi_image_free(this->imageData);
}

void	Scop::loadObjFile(std::string filePathName)
{
	std::ifstream objFile(filePathName);

	if (!objFile.is_open())
	{
		std::cout << "Error: could not open file" << std::endl;
		return ;
	}

	this->vertices.clear();
	this->vertexIndices.clear();
	this->normals.clear();
	this->textures.clear();

	std::string line;
	while (std::getline(objFile, line))
	{
		std::istringstream iss(line);
		std::string type;
		iss >> type;
		if (type == "v")
		{
			glm::vec3 vertex;
			iss >> vertex.x >> vertex.y >> vertex.z;
			this->vertices.push_back(vertex);
		}
		else if (type == "vn")
		{
			glm::vec3 normal;
			iss >> normal.x >> normal.y >> normal.z;
			this->normals.push_back(normal);
		}
		else if (type == "vt")
		{
			glm::vec2 texture;
			iss >> texture.x >> texture.y;
			this->textures.push_back(texture);
		}
		else if (type == "f")
		{
			std::vector<unsigned int> faceVertexIndices;
			std::vector<unsigned int> faceTextureIndices;
			std::vector<unsigned int> faceNormalIndices;

			unsigned int index;
			while (iss >> index)
			{
				faceVertexIndices.push_back(index - 1);

				if (iss.peek() == '/')
				{
					iss.ignore();
					if (iss.peek() != '/')
					{
						iss >> index;
						faceTextureIndices.push_back(index - 1);
					}
					if (iss.peek() == '/')
					{
						iss.ignore();
						iss >> index;
						faceNormalIndices.push_back(index - 1);
					}
				}
			}

			if (faceTextureIndices.empty())
				for (size_t i = 0; i < faceVertexIndices.size(); ++i)
					faceTextureIndices.push_back(0);

			for (size_t i = 1; i < faceVertexIndices.size() - 1; ++i)
			{
				this->vertexIndices.push_back(faceVertexIndices[0]);
				this->vertexIndices.push_back(faceVertexIndices[i]);
				this->vertexIndices.push_back(faceVertexIndices[i + 1]);
			}

			if (textures.empty())
			{
				for (unsigned int index : faceTextureIndices)
				{
					this->textures.push_back(glm::vec2(0.0f, 0.0f));
					this->textures[index] = glm::vec2(0.0f, 0.0f);
				}
			}
		}
	}

	if (normals.empty())
	{
		this->normals.resize(this->vertices.size(), glm::vec3(0.0f, 0.0f, 0.0f));

		for (size_t i = 0; i < this->vertexIndices.size(); i += 3)
		{
			glm::vec3 v1 = this->vertices[this->vertexIndices[i]];
			glm::vec3 v2 = this->vertices[this->vertexIndices[i + 1]];
			glm::vec3 v3 = this->vertices[this->vertexIndices[i + 2]];

			glm::vec3 normal = glm::normalize(glm::cross(v2 - v1, v3 - v1));

			this->normals[this->vertexIndices[i]] += normal;
			this->normals[this->vertexIndices[i + 1]] += normal;
			this->normals[this->vertexIndices[i + 2]] += normal;
		}

		for (size_t i = 0; i < this->normals.size(); ++i)
			this->normals[i] = glm::normalize(this->normals[i]);
	}

	glGenVertexArrays(1, &this->VAO);
	glGenBuffers(1, &this->VBO);
	glGenBuffers(1, &this->EBO);
	glGenBuffers(1, &this->normalVBO);
	glGenBuffers(1, &this->textureVBO);

	glBindVertexArray(this->VAO);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, vertexIndices.size() * sizeof(unsigned int), &vertexIndices[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
	glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(glm::vec3), &this->vertices[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, this->textureVBO);
	glBufferData(GL_ARRAY_BUFFER, this->textures.size() * sizeof(glm::vec2), &this->textures[0], GL_STATIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, this->normalVBO);
	glBufferData(GL_ARRAY_BUFFER, this->normals.size() * sizeof(glm::vec3), &this->normals[0], GL_STATIC_DRAW);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	this->vertexShader = glCreateShader(GL_VERTEX_SHADER);
	this->fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(this->vertexShader, 1, &vertexShaderSource, nullptr);
	glShaderSource(this->fragmentShader, 1, &fragmentShaderSource, nullptr);

	glCompileShader(this->vertexShader);
	glCompileShader(this->fragmentShader);

	this->shaderProgram = glCreateProgram();
	glAttachShader(this->shaderProgram, this->vertexShader);
	glAttachShader(this->shaderProgram, this->fragmentShader);
	glLinkProgram(this->shaderProgram);

	glDeleteShader(this->vertexShader);
	glDeleteShader(this->fragmentShader);
}

glm::vec3 Scop::calculateModelCenterOffset()
{
	glm::vec3 minCoords = glm::vec3(std::numeric_limits<float>::max());
	glm::vec3 maxCoords = glm::vec3(std::numeric_limits<float>::lowest());

	for (const glm::vec3& vertex : vertices)
	{
		minCoords = glm::min(minCoords, vertex);
		maxCoords = glm::max(maxCoords, vertex);
	}

	glm::vec3 center = (minCoords + maxCoords) * 0.5f;

	return -center;
}
