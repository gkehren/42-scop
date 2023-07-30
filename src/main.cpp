#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

// Dimensions de la fenêtre
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// Callback pour gérer les erreurs GLFW
void errorCallback(int error, const char* description) {
	(void)error;
	std::cerr << "Erreur GLFW : " << description << std::endl;
}

// Fonction pour initialiser OpenGL et GLFW
bool initOpenGL() {
	if (!glfwInit()) {
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

int main() {
	// Initialisation de GLFW et OpenGL
	if (!initOpenGL()) {
		return -1;
	}

	// Création de la fenêtre
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "scope", nullptr, nullptr);
	if (!window) {
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

	// Définition des coordonnées des sommets du cube
	float vertices[] = {
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
	unsigned int indices[] = {
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

	// Boucle principale
	while (!glfwWindowShouldClose(window)) {
		// Gestion des événements
		glfwPollEvents();

		// Effacez l'écran
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Dessinez le cube
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

		// Échangez les tampons (double buffering)
		glfwSwapBuffers(window);
	}

	// Supprimez les ressources
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);

	// Terminez GLFW
	glfwTerminate();

	return 0;
}
