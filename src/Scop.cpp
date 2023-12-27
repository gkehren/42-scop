#include "../include/Scop.hpp"
#include <stdexcept>

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
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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
	this->loadObjFile("/home/gkehren/42-scop/ressources/untitled.obj");
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
		glUseProgram(0);

		this->updateUI();

		glfwSwapBuffers(window);
	}
}
