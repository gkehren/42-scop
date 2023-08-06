#include "../include/Scop.hpp"

const char* vertexShaderSource = R"(
	#version 330 core
	layout (location = 0) in vec3 aPos;
	layout (location = 1) in vec3 aNormal;

	out vec3 FragPos;
	out vec3 Normal;

	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

	void main() {
		gl_Position = projection * view * model * vec4(aPos, 1.0);
		FragPos = vec3(model * vec4(aPos, 1.0));
		Normal = mat3(transpose(inverse(model))) * aNormal;
	}
)";

const char* fragmentShaderSource = R"(
	#version 330 core
	out vec4 FragColor;

	void main() {
        float gray = (float((gl_PrimitiveID) % 5) / 10.0) * 0.4 + 0.02;
		FragColor = vec4(gray, gray, gray, 1.0);
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

	this->window = glfwCreateWindow(this->windowWidth, this->windowHeight, "scope", nullptr, nullptr);
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
	// glEnable(GL_CULL_FACE);

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

	this->cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);  
	this->cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	this->cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
	this->cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	this->view = glm::lookAt(this->cameraPos, this->cameraTarget, this->cameraUp);
	this->projection = glm::perspective(glm::radians(45.0f), (float)this->windowWidth / (float)this->windowHeight, 0.1f, 100.0f);
	this->model = glm::mat4(1.0f);

	this->loadObjFile("/home/gkehren/Documents/scop/ressources/42.obj");
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

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		this->cameraMovement();

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(this->shaderProgram);
		glUniformMatrix4fv(glGetUniformLocation(this->shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(this->model));
		glUniformMatrix4fv(glGetUniformLocation(this->shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(this->view));
		glUniformMatrix4fv(glGetUniformLocation(this->shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(this->projection));

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

void	Scop::updateUI()
{
	float currentFrame = glfwGetTime();
	this->deltaTime = currentFrame - this->lastFrame;
	this->lastFrame = currentFrame;
	this->totalTime += this->deltaTime;
	this->frames++;

	if (this->totalTime >= this->updateInterval)
	{
		this->fps = static_cast<float>(this->frames) / this->totalTime;
		this->frames = 0;
		this->totalTime = 0.0f;
	}
	ImGui::Begin("scop");
	ImGui::Text("FPS : %.1f", this->fps);
	ImGui::Text("Camera position : (%.1f, %.1f, %.1f)", this->cameraPos.x, this->cameraPos.y, this->cameraPos.z);
	ImGui::Text("Camera front : (%.1f, %.1f, %.1f)", this->cameraFront.x, this->cameraFront.y, this->cameraFront.z);
	ImGui::Text("Camera target : (%.1f, %.1f, %.1f)", this->cameraTarget.x, this->cameraTarget.y, this->cameraTarget.z);
	ImGui::Text("Camera up : (%.1f, %.1f, %.1f)", this->cameraUp.x, this->cameraUp.y, this->cameraUp.z);
	ImGui::Text("Yaw : %.1f", this->yaw);
	ImGui::Text("Pitch : %.1f", this->pitch);
	if (ImGui::Button("Reset camera"))
	{
		this->yaw = -90.0f;
		this->pitch = 0.0f;
		this->distanceFromCube = 8.0f;
	}
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

	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void	Scop::loadObjFile(std::string filePathName)
{
	std::cout << "filePathName: " <<  filePathName << std::endl;
	std::ifstream objFile(filePathName);

	if (!objFile.is_open())
	{
		std::cout << "Error: could not open file" << std::endl;
		return ;
	}

	this->vertices.clear();
	this->vertexIndices.clear();

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
			std::cout << "Vertex: " << vertex.x << " " << vertex.y << " " << vertex.z << std::endl;
			this->vertices.push_back(vertex);
		}
		else if (type == "f")
		{
			unsigned int vertexIndex1, vertexIndex2, vertexIndex3;
			iss >> vertexIndex1 >> vertexIndex2 >> vertexIndex3;
			std::cout << "Face: " << vertexIndex1 << " " << vertexIndex2 << " " << vertexIndex3 << std::endl;
			vertexIndices.push_back(vertexIndex1 - 1);
			vertexIndices.push_back(vertexIndex2 - 1);
			vertexIndices.push_back(vertexIndex3 - 1);
		}
	}

	this->normals.clear();
	this->normals.resize(this->vertices.size(), glm::vec3(0.0f, 0.0f, 0.0f));

	for (size_t i = 0; i < vertexIndices.size(); i += 3)
	{
		glm::vec3 v1 = this->vertices[this->vertexIndices[i]];
		glm::vec3 v2 = this->vertices[this->vertexIndices[i + 1]];
		glm::vec3 v3 = this->vertices[this->vertexIndices[i + 2]];

		glm::vec3 normal = glm::normalize(glm::cross(v2 - v1, v3 - v1));

		this->normals[this->vertexIndices[i]] += normal;
		this->normals[this->vertexIndices[i + 1]] += normal;
		this->normals[this->vertexIndices[i + 2]] += normal;
	}

	for (size_t i = 0; i < this->normals.size(); i++)
		this->normals[i] = glm::normalize(this->normals[i]);

	std::cout << "Number of vertices: " << this->vertices.size() << std::endl;
	std::cout << "Number of faces: " << this->vertexIndices.size() / 3 << std::endl;
	std::cout << "Number of normals: " << this->normals.size() << std::endl;

	glGenVertexArrays(1, &this->VAO);
	glGenBuffers(1, &this->VBO);
	glGenBuffers(1, &this->EBO);
	glGenBuffers(1, &this->normalVBO);

	glBindVertexArray(this->VAO);

	glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
	glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(glm::vec3), &this->vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->vertexIndices.size() * sizeof(unsigned int), &this->vertexIndices[0], GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, this->normalVBO);
	glBufferData(GL_ARRAY_BUFFER, this->normals.size() * sizeof(glm::vec3), &this->normals[0], GL_STATIC_DRAW);
	glBindVertexArray(this->VAO);
	glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1); 

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

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