#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_opengl3.h"

// Dimensions de la fenêtre
const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;

// Callback pour gérer les erreurs GLFW
void errorCallback(int error, const char* description)
{
	(void)error;
	std::cerr << "Erreur GLFW : " << description << std::endl;
}

const char* vertexShaderSource = R"(
	#version 330 core
	layout (location = 0) in vec3 aPos;

	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

	void main() {
		gl_Position = projection * view * model * vec4(aPos, 1.0);
	}
)";

const char* fragmentShaderSource = R"(
	#version 330 core
	out vec4 FragColor;

	void main() {
		FragColor = vec4(1.0, 0.5, 0.2, 1.0);
	}
)";

// Fonction pour initialiser OpenGL et GLFW
bool initOpenGL()
{
	if (!glfwInit())
	{
		std::cerr << "Erreur lors de l'initialisation de GLFW" << std::endl;
		return false;
	}

	glfwSetErrorCallback(errorCallback);
	
	// Utilise OpenGL 3.3 Core Profile
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	return true;
}

int main()
{
	// Initialisation de GLFW et OpenGL
	if (!initOpenGL())
	{
		return -1;
	}

	// Création de la fenêtre
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "scope", nullptr, nullptr);
	if (!window)
	{
		std::cerr << "Erreur lors de la création de la fenêtre GLFW" << std::endl;
		glfwTerminate();
		return -1;
	}

	// Rendre le contexte OpenGL courant
	glfwMakeContextCurrent(window);

	// Initialise GLEW pour accéder aux fonctions OpenGL
	if (glewInit() != GLEW_OK) {
		std::cerr << "Erreur lors de l'initialisation de GLEW" << std::endl;
		glfwTerminate();
		return -1;
	}

	glEnable(GL_DEPTH_TEST);

	// Définition des coordonnées des sommets du cube
	float vertices[] =
	{
		// Face avant
		-0.5f, -0.5f,  0.5f,
		 0.5f, -0.5f,  0.5f,
		 0.5f,  0.5f,  0.5f,
		-0.5f,  0.5f,  0.5f,
		// Face arrière
		-0.5f, -0.5f, -0.5f,
		 0.5f, -0.5f, -0.5f,
		 0.5f,  0.5f, -0.5f,
		-0.5f,  0.5f, -0.5f,
	};

	// Définition de l'ordre des indices pour les faces du cube
	unsigned int indices[] =
	{
		0, 1, 2, // Triangle 1
		2, 3, 0, // Triangle 2
		1, 5, 6, // Triangle 3
		6, 2, 1, // Triangle 4
		7, 6, 5, // Triangle 5
		5, 4, 7, // Triangle 6
		4, 0, 3, // Triangle 7
		3, 7, 4, // Triangle 8
		4, 5, 1, // Triangle 9
		1, 0, 4, // Triangle 10
		3, 2, 6, // Triangle 11
		6, 7, 3  // Triangle 12
	};

	// Création du VAO (Vertex Array Object) et du VBO (Vertex Buffer Object)
	unsigned int VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	// Liez le VAO
	glBindVertexArray(VAO);

	// Liez le VBO et définissez les données des sommets
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Liez l'EBO et définissez les données des indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Configuration de l'attribut de position des sommets
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Libérez le VAO, le VBO et l'EBO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	unsigned int vertexShader, fragmentShader, shaderProgram;
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);

	glCompileShader(vertexShader);
	glCompileShader(fragmentShader);

	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	float deltaTime = 0.0f;
	float lastFrame = 0.0f;
	float totalTime = 0.0f;
	const float updateInterval = 0.2f;
	int frames = 0;

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	double lastX = WINDOW_WIDTH / 2.0f;
	double lastY = WINDOW_HEIGHT / 2.0f;
	bool firstMouse = true;
	float yaw = -90.0f;
	float pitch = 0.0f;
	float distanceFromCube = 3.0f;

	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);  // Position initiale de la caméra
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);  // Direction vers laquelle la caméra regarde initialement
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);  // Vecteur "haut" de la caméra

	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	// Boucle principale
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::ShowDemoWindow();

		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		totalTime += deltaTime;
		frames++;

		double xPos, yPos;
		glfwGetCursorPos(window, &xPos, &yPos);

		if (firstMouse)
		{
			lastX = xPos;
			lastY = yPos;
			firstMouse = false;
		}

		float xoffset = xPos - lastX;
		float yoffset = lastY - yPos;


		float sensitivity = 0.005f;
		xoffset *= sensitivity;
		yoffset *= sensitivity;

		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE)
		{
			xoffset = 0;
			yoffset = 0;
			lastX = xPos;
			lastY = yPos;
		}

		yaw += xoffset;
		pitch += yoffset;

		if (pitch > 89.0f)
			pitch = 89.0f;

		if (pitch < -89.0f)
			pitch = -89.0f;

		glm::vec3 front;
		front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		front.y = sin(glm::radians(pitch));
		front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		cameraFront = glm::normalize(front);

		cameraPos = cameraTarget - distanceFromCube * cameraFront;

		glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
		glm::mat4 model = glm::mat4(1.0f);

		// Effacez l'écran
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(shaderProgram);
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		if (totalTime >= updateInterval)
		{
			float fps = static_cast<float>(frames) / totalTime;
			ImGui::SetNextWindowPos(ImVec2(10, 10));
			ImGui::Begin("FPS Display", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
			ImGui::Text("FPS : %.0f", fps);
			ImGui::End();
			frames = 0;
			totalTime = 0.0f;
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	
		glfwSwapBuffers(window);
	}

	// Supprimez les ressources
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	// Terminez GLFW
	glfwTerminate();

	return 0;
}
