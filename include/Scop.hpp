#pragma once

#include "./glew/glew.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include "struct.hpp"
#include "../imgui/imgui.h"
#include "../imgui/ImGuiFileDialog.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_opengl3.h"

typedef unsigned int uint;
typedef unsigned short ushort;

class Scop
{
	public:
		Scop();
		~Scop();
		void run();

		void	processMouseScroll(double yoffset);

		static void	scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

	private:
		GLFWwindow*	window;

		int			windowWidth;
		int			windowHeight;

		float		deltaTime;
		float		lastFrame;
		float		totalTime;
		float		updateInterval;
		int			frames;

		double		lastX;
		double		lastY;
		bool		firstMouse;
		float		yaw;
		float		pitch;
		float		distanceFromCube;
		float		fps;
		float		sensitivity;
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

		// gradient
		bool		showGradient;
		Vec3		gradientStartColor;
		Vec3		gradientEndColor;

		// soft transition
		bool		transition;
		bool		previousShowTextures;
		float		transitionFactor;
		float		transitionStartTime;
		float		transitionDuration;

		float		movementSpeed;
		float		rotationSpeed;
		float		rotationAngle;
		Vec3		objectPosition;

		bool		showTextures;
		bool		showWireframe;
		bool		showLight;

		uint		textureID;

		uint		vertexShader;
		uint		fragmentShader;
		uint		shaderProgram;

		uint		VBO; // vertex buffer object
		uint		EBO; // element buffer object
		uint		VAO; // vertex array object
		uint		textureVBO; // texture vertex buffer object
		uint		normalVBO; // normal vertex buffer object

		std::vector<Vec3>			vertex_postitions;
		std::vector<TextureCoord>	vertex_texcoords;
		std::vector<Vec3>			vertex_normals;
		std::vector<uint>			indices;

		void		loadShader();
		void		cameraMovement();
		void		objectMovement();
		void		updateUI();
		void		createBuffersAndArrays();
		void		loadObjFile(const char* filePathName);
		void		loadTexture(const char* filename);
		void		indexVBO(std::vector<Vec3> &out_vertices, std::vector<TextureCoord> &out_uvs, std::vector<Vec3> &out_normals);
		Vec3		calculateModelCenterOffset();
		float		toRadians(float degrees);
};
