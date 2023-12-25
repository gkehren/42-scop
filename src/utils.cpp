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

float	Scop::toRadians(float degrees)
{
	return degrees * M_PI / 180.0f;
}
