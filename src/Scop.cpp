#include "../include/Scop.hpp"
#include <stdexcept>
#define STB_IMAGE_IMPLEMENTATION
#include "../imgui/stb_image.h"

Mat4::Mat4(const Mat3& mat3) {
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			(*this)(i, j) = mat3(i, j);

	(*this)(3, 0) = (*this)(3, 1) = (*this)(3, 2) = 0.0f;
	(*this)(0, 3) = (*this)(1, 3) = (*this)(2, 3) = 0.0f;
	(*this)(3, 3) = 1.0f;
}

Mat3 operator*(float scalar, const Mat3& matrix) {
	return matrix * scalar;
}

Vec3 operator*(float scalar, const Vec3& vec) {
	return vec * scalar;
}

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
	// Initialize GLFW
	if (!glfwInit())
	{
		std::cerr << "Error while initializing GLFW" << std::endl;
		throw std::runtime_error("Error while initializing GLFW");
	}

	glfwSetErrorCallback(errorCallback);

	// Use OpenGL 3.3 Core Profile
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

	// Create window
	this->windowWidth = 1920;
	this->windowHeight = 1080;
	this->window = glfwCreateWindow(this->windowWidth, this->windowHeight, "scop", nullptr, nullptr);
	if (!this->window)
	{
		std::cerr << "Error while creating GLFW window" << std::endl;
		glfwTerminate();
		throw std::runtime_error("Error while creating GLFW window");
	}

	glfwMakeContextCurrent(this->window);

	// Initialize GLEW
	if (glewInit() != GLEW_OK)
	{
		std::cerr << "Error while initializing GLEW" << std::endl;
		glfwTerminate();
		throw std::runtime_error("Error while initializing GLEW");
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
	this->showLight = false;

	this->cameraPos = Vec3(0.0f, 0.0f, 3.0f);
	this->cameraFront = Vec3(0.0f, 0.0f, -1.0f);
	this->cameraTarget = Vec3(0.0f, 0.0f, 0.0f);
	this->cameraUp = Vec3(0.0f, 1.0f, 0.0f);

	this->view = Mat4::lookAt(this->cameraPos, this->cameraTarget, this->cameraUp);
	this->projection = Mat4::perspective(toRadians(45.0f), (float)this->windowWidth / (float)this->windowHeight, 0.1f, 100.0f);
	this->model = Mat4();

	this->lightColor = Vec3(1.0f, 1.0f, 1.0f);
	this->lightPos = Vec3(0.0f, 10.0f, 0.0f);
	this->objectColor = Vec3(1.0f, 0.5f, 0.31f);

	this->movementSpeed = 0.05f;
	this->rotationSpeed = 0.5f;
	this->rotationAngle = 0.0f;
	this->objectPosition = Vec3(0.0f, 0.0f, 0.0f);

	this->transition = false;
	this->previousShowTextures = false;
	this->transitionFactor = 0.0f;
	this->transitionStartTime = 0.0f;
	this->transitionDuration = 1.0f;

	this->showGradient = true;
	this->gradientStartColor = Vec3(0.0f, 0.0f, 0.0f);
	this->gradientEndColor = Vec3(1.0f, 1.0f, 1.0f);

	this->loadShader();

	this->loadTexture("/home/gkehren/42-scop/ressources/brick.bmp");
	this->loadObjFile("/home/gkehren/42-scop/ressources/42.obj");
}

Scop::~Scop()
{
	glDeleteProgram(shaderProgram);

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteBuffers(1, &textureVBO);
	glDeleteBuffers(1, &normalVBO);
	glDeleteTextures(1, &textureID);

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
		glUniformMatrix4fv(glGetUniformLocation(this->shaderProgram, "model"), 1, GL_FALSE, Mat4::value_ptr(this->model));
		glUniformMatrix4fv(glGetUniformLocation(this->shaderProgram, "view"), 1, GL_FALSE, Mat4::value_ptr(this->view));
		glUniformMatrix4fv(glGetUniformLocation(this->shaderProgram, "projection"), 1, GL_FALSE, Mat4::value_ptr(this->projection));
		glUniform3fv(glGetUniformLocation(this->shaderProgram, "lightColor"), 1, Vec3::value_ptr(this->lightColor));
		glUniform3fv(glGetUniformLocation(this->shaderProgram, "lightPos"), 1, Vec3::value_ptr(this->lightPos));
		glUniform3fv(glGetUniformLocation(this->shaderProgram, "objectColor"), 1, Vec3::value_ptr(this->objectColor));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glUniform1i(glGetUniformLocation(this->shaderProgram, "textureSampler"), 0);
		glUniform1i(glGetUniformLocation(this->shaderProgram, "showLight"), this->showLight);
		glUniform1f(glGetUniformLocation(this->shaderProgram, "transitionFactor"), this->transitionFactor);
		glUniform1i(glGetUniformLocation(this->shaderProgram, "showGradient"), this->showGradient);
		glUniform3fv(glGetUniformLocation(this->shaderProgram, "gradientStartColor"), 1, Vec3::value_ptr(this->gradientStartColor));
		glUniform3fv(glGetUniformLocation(this->shaderProgram, "gradientEndColor"), 1, Vec3::value_ptr(this->gradientEndColor));

		if (showWireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glBindVertexArray(this->VAO);
		glDrawElements(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, 0);
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

	Vec3 front(0.0f, 0.0f, 0.0f);
	front.x = cos(toRadians(this->yaw)) * cos(toRadians(this->pitch));
	front.y = sin(toRadians(this->pitch));
	front.z = sin(toRadians(this->yaw)) * cos(toRadians(this->pitch));
	this->cameraFront = Vec3::normalize(front);

	this->cameraPos = this->cameraTarget - this->cameraFront * this->distanceFromCube;
	this->view = Mat4::lookAt(this->cameraPos, this->cameraTarget, this->cameraUp);
}

void Scop::objectMovement()
{
	Mat3 rotationMatrix3 = Mat3(this->model);
	Mat3 invRotationMatrix = Mat3::transpose(rotationMatrix3);

	Vec3 translationDelta(0.0f, 0.0f, 0.0f);

	if (glfwGetKey(this->window, GLFW_KEY_W) == GLFW_PRESS)
		translationDelta += movementSpeed * invRotationMatrix * Vec3(0.0f, 0.0f, -1.0f);
	if (glfwGetKey(this->window, GLFW_KEY_S) == GLFW_PRESS)
		translationDelta -= movementSpeed * invRotationMatrix * Vec3(0.0f, 0.0f, -1.0f);
	if (glfwGetKey(this->window, GLFW_KEY_A) == GLFW_PRESS)
		translationDelta -= movementSpeed * invRotationMatrix * Vec3(1.0f, 0.0f, 0.0f);
	if (glfwGetKey(this->window, GLFW_KEY_D) == GLFW_PRESS)
		translationDelta += movementSpeed * invRotationMatrix * Vec3(1.0f, 0.0f, 0.0f);
	if (glfwGetKey(this->window, GLFW_KEY_Q) == GLFW_PRESS)
		translationDelta += movementSpeed * Vec3(0.0f, 1.0f, 0.0f);
	if (glfwGetKey(this->window, GLFW_KEY_E) == GLFW_PRESS)
		translationDelta -= movementSpeed * Vec3(0.0f, 1.0f, 0.0f);
	if (glfwGetKey(this->window, GLFW_KEY_SPACE) == GLFW_PRESS)
		translationDelta = -objectPosition;

	this->objectPosition += translationDelta;
	this->rotationAngle = this->rotationSpeed * this->deltaTime;

	Vec3 modelCenterOffset = this->calculateModelCenterOffset() - this->objectPosition;
	Mat4 translationToOrigin = Mat4::translate(-modelCenterOffset);
	Mat4 translationBack = Mat4::translate(modelCenterOffset);
	Mat4 rotationMatrix4 = Mat4::rotateY(this->rotationAngle);

	this->model = Mat4::translate(this->objectPosition) * translationBack * Mat4(rotationMatrix3) * rotationMatrix4 * translationToOrigin;
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
	if (ImGui::Button("Load 3D Model"))
		ImGuiFileDialog::Instance()->OpenDialog("ChooseObjDlgKey", "Choose File", ".obj", ".");

	if (ImGuiFileDialog::Instance()->Display("ChooseObjDlgKey"))
	{
		if (ImGuiFileDialog::Instance()->IsOk())
		{
			std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
			this->loadObjFile(filePathName);
		}
		ImGuiFileDialog::Instance()->Close();
	}
	if (ImGui::Button("Reset object"))
		this->objectPosition = Vec3(0.0f, 0.0f, 0.0f);
	if (ImGui::Button("Reset camera"))
	{
		this->yaw = -90.0f;
		this->pitch = 0.0f;
		this->distanceFromCube = 8.0f;
	}
	ImGui::SliderFloat("Rotation Speed", &this->rotationSpeed, 0.0f, 2.0f);
	ImGui::Checkbox("Wireframe", &this->showWireframe);
	ImGui::Checkbox("Texture", &this->showTextures);
	if (ImGui::Button("Load Texture"))
		ImGuiFileDialog::Instance()->OpenDialog("ChooseTextureDlgKey", "Choose File", ".bmp", ".");

	if (ImGuiFileDialog::Instance()->Display("ChooseTextureDlgKey"))
	{
		if (ImGuiFileDialog::Instance()->IsOk())
		{
			std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
			this->loadTexture(filePathName.c_str());
		}
		ImGuiFileDialog::Instance()->Close();
	}
	ImGui::Checkbox("Light", &this->showLight);
	ImGui::ColorEdit3("Object Color", &this->objectColor.x);
	ImGui::ColorEdit3("Light Color", &this->lightColor.x);
	ImGui::Checkbox("Gradient", &this->showGradient);
	ImGui::ColorEdit3("Gradient Start Color", &this->gradientStartColor.x);
	ImGui::ColorEdit3("Gradient End Color", &this->gradientEndColor.x);

	if (this->showTextures != this->previousShowTextures)
	{
		this->transitionStartTime = glfwGetTime();
		this->transition = true;
	}

	if (this->transition && this->showTextures)
	{
		float transitionTime = glfwGetTime() - this->transitionStartTime;
		this->transitionFactor = transitionTime / this->transitionDuration;

		if (this->transitionFactor >= 1.0f)
		{
			this->transitionFactor = 1.0f;
			this->transition = false;
		}
	}
	else if (this->transition && !this->showTextures)
	{
		float transitionTime = glfwGetTime() - this->transitionStartTime;
		this->transitionFactor = 1.0f - transitionTime / this->transitionDuration;

		if (this->transitionFactor <= 0.0f)
		{
			this->transitionFactor = 0.0f;
			this->transition = false;
		}
	}
	this->previousShowTextures = this->showTextures;

	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void	Scop::loadTexture(const char* filename)
{
	glDeleteTextures(1, &this->textureID);
	this->textureID = 0;

	int	width, height, channels;
	unsigned char* data = stbi_load(filename, &width, &height, &channels, 0);
	if (!data)
	{
		std::cerr << "Error: could not load texture" << std::endl;
		throw std::runtime_error("Error: could not load texture");
	}

	glGenTextures(1, &this->textureID);
	glBindTexture(GL_TEXTURE_2D, this->textureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(data);
}

void	Scop::loadObjFile(std::string filePathName)
{
	std::ifstream objFile(filePathName);

	if (!objFile.is_open())
	{
		std::cerr << "Error: could not open file" << std::endl;
		throw std::runtime_error("Error: could not open file");
	}

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
			Vec3 vertex;
			iss >> vertex.x >> vertex.y >> vertex.z;
			this->vertices.push_back(vertex);
		}
		else if (lineType == "vn")
		{
			Vec3 normal;
			iss >> normal.x >> normal.y >> normal.z;
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
				if (!indexToken.empty())
					face.vertexIndex[i] = std::stoi(indexToken) - 1;

				if (getline(tokenStream, indexToken, '/') && !indexToken.empty())
					face.textureCoordIndex[i] = std::stoi(indexToken) - 1;
				else
					face.textureCoordIndex[i] = -1;

				if (getline(tokenStream, indexToken) && !indexToken.empty())
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
				if (!indexToken.empty())
					secondFace.vertexIndex[2] = std::stoi(indexToken) - 1;

				if (getline(tokenStream, indexToken, '/') && !indexToken.empty())
					secondFace.textureCoordIndex[2] = std::stoi(indexToken) - 1;
				else
					secondFace.textureCoordIndex[2] = -1;

				if (getline(tokenStream, indexToken) && !indexToken.empty())
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

	for (const auto& face : faces)
		for (int i = 0; i < 3; ++i)
			this->indices.push_back(face.vertexIndex[i]);

	if (normals.size() == 0)
	{
		normals.resize(vertices.size(), Vec3(0.0f, 0.0f, 0.0f));
		for (int i = 0; i < indices.size(); i += 3)
		{
			Vec3 v0 = vertices[indices[i]];
			Vec3 v1 = vertices[indices[i + 1]];
			Vec3 v2 = vertices[indices[i + 2]];

			Vec3 edge1 = v1 - v0;
			Vec3 edge2 = v2 - v0;

			Vec3 normal = Vec3::cross(edge1, edge2);

			normals[indices[i]] = normal;
			normals[indices[i + 1]] = normal;
			normals[indices[i + 2]] = normal;
		}

		for (auto& normal : normals)
			normal = normal.normalize();
	}

	if (textureCoords.size() == 0)
	{
		for (const auto& vertice : vertices)
			textureCoords.push_back({ vertice.x, vertice.y });
	}

	glGenVertexArrays(1, &this->VAO);
	glGenBuffers(1, &this->VBO);
	glGenBuffers(1, &this->EBO);
	glGenBuffers(1, &this->normalVBO);
	glGenBuffers(1, &this->textureVBO);

	glBindVertexArray(this->VAO);

	// Fill EBO with indices data
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(uint), this->indices.data(), GL_STATIC_DRAW);

	// Attribute 0: vertex position
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vec3), vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	// Attribute 1: texture coordinates
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, this->textureVBO);
	glBufferData(GL_ARRAY_BUFFER, this->textureCoords.size() * sizeof(TextureCoord), this->textureCoords.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

	// Attribute 2: normals
	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, this->normalVBO);
	glBufferData(GL_ARRAY_BUFFER, this->normals.size() * sizeof(Vec3), this->normals.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

	glBindVertexArray(0);

	cameraTarget = calculateModelCenterOffset();
}

Vec3 Scop::calculateModelCenterOffset()
{
	Vec3 min = Vec3(FLT_MAX, FLT_MAX, FLT_MAX);
	Vec3 max = Vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (const Vec3& vertex : vertices)
	{
		min = Vec3::min(min, vertex);
		max = Vec3::max(max, vertex);
	}

	Vec3 center = (min + max) * 0.5f;

	return -center;
}

float	Scop::toRadians(float degrees)
{
	return degrees * M_PI / 180.0f;
}
