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
#include "struct.hpp"

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
		Vec3		cameraPos;
		Vec3		cameraFront;
		Vec3		cameraTarget;
		Vec3		cameraUp;
		Mat4		view;
		Mat4		projection;
		Mat4		model;
		Vec3		lightPos;
		Vec3		lightColor;
		Vec3		objectColor;

		float		movementSpeed;
		float		rotationSpeed;
		float		rotationAngle;
		Vec3		objectPosition;

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

		std::vector<Vec3>			vertices;
		std::vector<Normal>			normals;
		std::vector<TextureCoord>	textureCoords;
		std::vector<Face>			faces;
		std::vector<unsigned int>	indices;

		void		cameraMovement();
		void		objectMovement();
		void		updateUI();
		void		loadObjFile(std::string filePathName);
		void		loadbmpFile(std::string filePathName);
		Vec3		calculateModelCenterOffset();
};
