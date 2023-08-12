#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_opengl3.h"
#include "../imgui/ImGuiFileDialog.h"
#include <fstream>

class Scop
{
	public:
		Scop();
		~Scop();
		void run();

		void	processMouseScroll(double yoffset);

		static void	scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

	private:
		GLFWwindow* window;

		int		windowWidth;
		int		windowHeight;

		float deltaTime;
		float lastFrame;
		float totalTime;
		float updateInterval;
		int frames;

		double lastX;
		double lastY;
		bool firstMouse;
		float yaw;
		float pitch;
		float distanceFromCube;
		float fps;
		float sensitivity;
		glm::vec3 cameraPos;
		glm::vec3 cameraFront;
		glm::vec3 cameraTarget;
		glm::vec3 cameraUp;
		glm::mat4 view;
		glm::mat4 projection;
		glm::mat4 model;
		glm::vec3 lightPos;
		glm::vec3 lightColor;
		glm::vec3 objectColor;

		float movementSpeed;
		float rotationSpeed;
		float rotationAngle;
		glm::vec3 objectPosition;

		unsigned int VAO;
		unsigned int VBO;
		unsigned int EBO;
		GLuint normalVBO;
		GLuint textureVBO;

		bool	showTextures;
		bool	showWireframe;
		bool	showGradient;
		bool	showLight;

		// Texture
		unsigned char *imageData;
		int imageWidth;
		int imageHeight;
		int imageChannels;
		GLuint textureID;

		unsigned int vertexShader;
		unsigned int fragmentShader;
		unsigned int shaderProgram;

		std::vector<glm::vec3> vertices;
		std::vector<unsigned int> vertexIndices;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec2> textures;

		void	cameraMovement();
		void	objectMovement();
		void	updateUI();
		void	loadObjFile(std::string filePathName);
		void	loadbmpFile(std::string filePathName);
		glm::vec3	calculateModelCenterOffset();
};
