#include "../include/Scop.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "../imgui/stb_image.h"

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
			this->loadObjFile(filePathName.c_str());
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(data);
}

void	Scop::loadObjFile(const char* filePathName)
{
	std::ifstream objFile(filePathName, std::ios::in);

	if (!objFile.is_open())
	{
		std::cerr << "Error: could not open file" << std::endl;
		throw std::runtime_error("Error: could not open file");
	}

	this->vertex_postitions.clear();
	this->vertex_texcoords.clear();
	this->vertex_normals.clear();
	this->indices.clear();
	glDeleteBuffers(1, &this->VBO);
	glDeleteBuffers(1, &this->EBO);
	glDeleteVertexArrays(1, &this->VAO);
	glDeleteBuffers(1, &this->textureVBO);
	glDeleteBuffers(1, &this->normalVBO);

	std::string line;
	while (std::getline(objFile, line))
	{
		std::istringstream iss(line);
		std::string type;
		iss >> type;

		if (type == "v")
		{
			Vec3 vertex;
			iss >> vertex.x >> vertex.y >> vertex.z;
			this->vertex_postitions.push_back(vertex);
		}
		//else if (type == "vt")
		//{
		//	TextureCoord texture;
		//	iss >> texture.u >> texture.v;
		//	this->vertex_texcoords.push_back(texture);
		//}
		else if (type == "vn")
		{
			Vec3 normal;
			iss >> normal.x >> normal.y >> normal.z;
			this->vertex_normals.push_back(normal);
		}
		else if (type == "f")
		{
			uint vp_index[3], vt_index[3], vn_index[3];
			std::string token;

			for (uint i = 0; i < 3; ++i)
			{
				iss >> token;

				std::istringstream tokenStream(token);
				std::string indexToken;

				getline(tokenStream, indexToken, '/');
				if (!indexToken.empty())
					vp_index[i] = std::stoi(indexToken) - 1;

				if (getline(tokenStream, indexToken, '/') && !indexToken.empty())
					vt_index[i] = std::stoi(indexToken) - 1;

				if (getline(tokenStream, indexToken, '/') && !indexToken.empty())
					vn_index[i] = std::stoi(indexToken) - 1;
			}

			this->indices.push_back(vp_index[0]);
			this->indices.push_back(vp_index[1]);
			this->indices.push_back(vp_index[2]);

			if (iss >> token)
			{
				this->indices.push_back(vp_index[0]);
				this->indices.push_back(vp_index[2]);
				this->indices.push_back(std::stoi(token) - 1);
			}
		}
	}
	objFile.close();

	if (this->vertex_texcoords.empty())
	{
		for (uint i = 0; i < this->indices.size(); i+= 3)
		{
			float u = indices[i];
			float v = indices[i + 1];
			this->vertex_texcoords.push_back({ u, v });
		}
	}

	std::cout << "Number of vertices: " << this->vertex_postitions.size() << std::endl;
	std::cout << "Number of texture coordinates: " << this->vertex_texcoords.size() << std::endl;
	std::cout << "Number of normals: " << this->vertex_normals.size() << std::endl;
	std::cout << "Number of indices: " << this->indices.size() << std::endl;

	createBuffersAndArrays();
}

void	Scop::createBuffersAndArrays()
{
	// Generate Vertex Array Object
	glGenVertexArrays(1, &this->VAO);
	glBindVertexArray(VAO);

	// Generate and bind the VBO for positions
	glGenBuffers(1, &this->VBO);
	glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
	glBufferData(GL_ARRAY_BUFFER, this->vertex_postitions.size() * sizeof(Vec3), this->vertex_postitions.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3), (void*)0);
	glEnableVertexAttribArray(0);

	// Generate and bind the VBO for texture coordinates
	glGenBuffers(1, &this->textureVBO);
	glBindBuffer(GL_ARRAY_BUFFER, this->textureVBO);
	glBufferData(GL_ARRAY_BUFFER, this->vertex_texcoords.size() * sizeof(TextureCoord), this->vertex_texcoords.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TextureCoord), (void*)0);
	glEnableVertexAttribArray(1);

	// Generate and bind the VBO for normals
	glGenBuffers(1, &this->normalVBO);
	glBindBuffer(GL_ARRAY_BUFFER, this->normalVBO);
	glBufferData(GL_ARRAY_BUFFER, this->vertex_normals.size() * sizeof(Vec3), this->vertex_normals.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3), (void*)0);
	glEnableVertexAttribArray(2);

	// Generate and bind the EBO for indices
	glGenBuffers(1, &this->EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(uint), this->indices.data(), GL_STATIC_DRAW);

	glBindVertexArray(0);

	this->cameraTarget = calculateModelCenterOffset();
}

float	Scop::toRadians(float degrees)
{
	return degrees * M_PI / 180.0f;
}
