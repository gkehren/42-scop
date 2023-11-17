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
#include <cmath>

struct Vertex {
	float x, y, z;
};

struct Normal {
	float nx, ny, nz;
};

struct TextureCoord {
	float u, v;
};

struct Face {
	int vertexIndex[4];
	int normalIndex[4];
	int textureCoordIndex[4];
};

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

		float		movementSpeed;
		float		rotationSpeed;
		float		rotationAngle;
		glm::vec3	objectPosition;

		GLuint	VAO;
		GLuint	VBO;
		GLuint	EBO;

		bool	showTextures;
		bool	showWireframe;
		bool	showGradient;
		bool	showLight;

		// Texture
		unsigned char	*imageData;
		int				imageWidth;
		int				imageHeight;
		int				imageChannels;
		GLuint			textureID;

		GLuint	vertexShader;
		GLuint	fragmentShader;
		GLuint	shaderProgram;
		GLuint	textureVBO;
		GLuint	normalVBO;

		std::vector<Vertex>			vertices;
		std::vector<Normal>			normals;
		std::vector<TextureCoord>	textureCoords;
		std::vector<Face>			faces;
		std::vector<unsigned int>	indices;

		void		cameraMovement();
		void		objectMovement();
		void		updateUI();
		void		loadObjFile(std::string filePathName);
		void		loadbmpFile(std::string filePathName);
		glm::vec3	calculateModelCenterOffset();
};
