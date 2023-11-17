#include "../include/Scop.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "../imgui/stb_image.h"

const char* vertexShaderSource = R"(
	#version 420 core

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
		gl_Position = projection * view * model * vec4(inPosition, 1.0);
		TexCoord = inTexCoord;
		FragPos = vec3(model * vec4(inPosition, 1.0));
		Normal = mat3(transpose(inverse(model))) * inNormal;
	}
)";

const char* fragmentShaderSource = R"(
	#version 420 core

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

	// Use OpenGL 4.2 Core Profile
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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

	// LOAD SHADER
	this->vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
	glCompileShader(vertexShader);

	GLint success;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		char infoLog[512];
		glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
		std::cout << "Error compiling vertex shader:\n" << infoLog << std::endl;
	}

	this->fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
	glCompileShader(fragmentShader);

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		char infoLog[512];
		glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
		std::cout << "Error compiling fragment shader:\n" << infoLog << std::endl;
	}

	this->shaderProgram = glCreateProgram();
	glAttachShader(this->shaderProgram, this->vertexShader);
	glAttachShader(this->shaderProgram, this->fragmentShader);
	glLinkProgram(this->shaderProgram);

	glGetProgramiv(this->shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		char infoLog[512];
		glGetProgramInfoLog(this->shaderProgram, 512, nullptr, infoLog);
		std::cout << "Error linking shader program:\n" << infoLog << std::endl;
	}

	glDeleteShader(this->vertexShader);
	glDeleteShader(this->fragmentShader);
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

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glUseProgram(this->shaderProgram);
		glUniformMatrix4fv(glGetUniformLocation(this->shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(this->model));
		glUniformMatrix4fv(glGetUniformLocation(this->shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(this->view));
		glUniformMatrix4fv(glGetUniformLocation(this->shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(this->projection));
		glUniform3fv(glGetUniformLocation(this->shaderProgram, "lightColor"), 1, glm::value_ptr(this->lightColor));
		glUniform3fv(glGetUniformLocation(this->shaderProgram, "lightPos"), 1, glm::value_ptr(this->lightPos));
		glUniform3fv(glGetUniformLocation(this->shaderProgram, "objectColor"), 1, glm::value_ptr(this->objectColor));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glUniform1i(glGetUniformLocation(this->shaderProgram, "textureSampler"), 0);
		glUniform1i(glGetUniformLocation(this->shaderProgram, "showTextures"), this->showTextures);
		glUniform1i(glGetUniformLocation(this->shaderProgram, "showGradient"), this->showGradient);
		glUniform1i(glGetUniformLocation(this->shaderProgram, "showLight"), this->showLight);

		if (showWireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glBindVertexArray(this->VAO);
		glDrawElements(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		glUseProgram(0);

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

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, this->imageWidth, this->imageHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, this->imageData);
	glGenerateMipmap(GL_TEXTURE_2D);

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

	std::cout << "Loading file: " << filePathName << std::endl;

	this->vertices.clear();
	this->normals.clear();
	this->textureCoords.clear();
	this->faces.clear();
	this->indices.clear();
	glDeleteVertexArrays(1, &this->VAO);
	glDeleteBuffers(1, &this->VBO);
	glDeleteBuffers(1, &this->EBO);
	glDeleteBuffers(1, &this->textureVBO);
	glDeleteBuffers(1, &this->normalVBO);

	std::string line;
	while (std::getline(objFile, line))
	{
		std::istringstream iss(line);
		std::string lineType;
		iss >> lineType;
		if (lineType == "v")
		{
			Vertex vertex;
			iss >> vertex.x >> vertex.y >> vertex.z;
			this->vertices.push_back(vertex);
		}
		else if (lineType == "vn")
		{
			Normal normal;
			iss >> normal.nx >> normal.ny >> normal.nz;
			this->normals.push_back(normal);
		}
		else if (lineType == "vt")
		{
			TextureCoord textureCoord;
			iss >> textureCoord.u >> textureCoord.v;
			this->textureCoords.push_back(textureCoord);
		}
		else if (lineType == "f")
		{
			Face face;
			char slash;
			std::string token;

			for (int i = 0; i < 3; ++i)
			{
				iss >> token;

				std::istringstream tokenStream(token);
				std::string indexToken;

				getline(tokenStream, indexToken, '/');
				face.vertexIndex[i] = std::stoi(indexToken) - 1;

				if (getline(tokenStream, indexToken, '/'))
					face.textureCoordIndex[i] = std::stoi(indexToken) - 1;
				else
					face.textureCoordIndex[i] = -1;

				if (getline(tokenStream, indexToken))
					face.normalIndex[i] = std::stoi(indexToken) - 1;
				else
					face.normalIndex[i] = -1;
			}

			faces.push_back(face);

			if (iss >> token)
			{
				Face secondFace;
				secondFace.vertexIndex[0] = face.vertexIndex[0];
				secondFace.textureCoordIndex[0] = face.textureCoordIndex[0];
				secondFace.normalIndex[0] = face.normalIndex[0];

				std::istringstream tokenStream(token);
				std::string indexToken;

				getline(tokenStream, indexToken, '/');
				secondFace.vertexIndex[2] = std::stoi(indexToken) - 1;

				if (getline(tokenStream, indexToken, '/'))
					secondFace.textureCoordIndex[2] = std::stoi(indexToken) - 1;
				else
					secondFace.textureCoordIndex[2] = -1;

				if (getline(tokenStream, indexToken))
					secondFace.normalIndex[2] = std::stoi(indexToken) - 1;
				else
					secondFace.normalIndex[2] = -1;

				secondFace.vertexIndex[1] = face.vertexIndex[2];
				secondFace.textureCoordIndex[1] = face.textureCoordIndex[2];
				secondFace.normalIndex[1] = face.normalIndex[2];

				faces.push_back(secondFace);
			}
		}
	}

	if (textureCoords.size() == 0)
	{
		std::cout << "No texture coordinates found, using default texture coordinates" << std::endl;
		textureCoords.resize(vertices.size());

		for (size_t i = 0; i < vertices.size(); i++)
		{
			float u = std::atan2(vertices[i].z, vertices[i].x) / (2.0f * M_PI) + 0.5f;
			float v = asinf(vertices[i].y) / M_PI + 0.5f;
			textureCoords[i] = { u, v };
		}
	}

	for (const auto& face : faces)
		for (int i = 0; i < 3; ++i)
			this->indices.push_back(face.vertexIndex[i]);

	glGenVertexArrays(1, &this->VAO);
	glGenBuffers(1, &this->VBO);
	glGenBuffers(1, &this->EBO);
	glGenBuffers(1, &this->normalVBO);
	glGenBuffers(1, &this->textureVBO);

	glBindVertexArray(this->VAO);

	// Fill EBO with indices data
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(unsigned int), this->indices.data(), GL_STATIC_DRAW);

	// Attribute 0: vertex position
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);


	// Attribute 1: texture coordinates
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, this->textureVBO);
	glBufferData(GL_ARRAY_BUFFER, this->textureCoords.size() * sizeof(TextureCoord), this->textureCoords.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

	// Attribute 2: normals
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, this->normalVBO);
	glBufferData(GL_ARRAY_BUFFER, this->normals.size() * sizeof(Normal), this->normals.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

	glBindVertexArray(0);
}


glm::vec3 Scop::calculateModelCenterOffset()
{
	glm::vec3 minCoords = glm::vec3(std::numeric_limits<float>::max());
	glm::vec3 maxCoords = glm::vec3(std::numeric_limits<float>::lowest());

	for (const auto& vertex : vertices)
	{
		minCoords = glm::min(minCoords, vertex);
		maxCoords = glm::max(maxCoords, vertex);
	}

	glm::vec3 center = (minCoords + maxCoords) * 0.5f;

	return -center;
}
