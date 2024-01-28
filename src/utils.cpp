#include "../include/Scop.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "../imgui/stb_image.h"

void	Scop::updateUI()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

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

	glDeleteBuffers(1, &this->VBO);
	glDeleteBuffers(1, &this->EBO);
	glDeleteVertexArrays(1, &this->VAO);
	glDeleteBuffers(1, &this->textureVBO);
	glDeleteBuffers(1, &this->normalVBO);

	this->vertex_postitions.clear();
	this->vertex_texcoords.clear();
	this->vertex_normals.clear();
	this->indices.clear();

	std::vector<uint> vertexIndices, uvIndices, normalIndices;
	std::vector<Vec3> temp_vertices, out_vertices;
	std::vector<TextureCoord> temp_uvs, out_uvs;
	std::vector<Vec3> temp_normals, out_normals;

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
			temp_vertices.push_back(vertex);
		}
		else if (type == "vt")
		{
			TextureCoord texture;
			iss >> texture.u >> texture.v;
			temp_uvs.push_back(texture);
		}
		else if (type == "vn")
		{
			Vec3 normal;
			iss >> normal.x >> normal.y >> normal.z;
			temp_normals.push_back(normal);
		}
		else if (type == "f")
		{
			uint vp_index[3], vt_index[3], vn_index[3];
			std::string token;

			for (uint i = 0; i < 3; i++)
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

			vertexIndices.push_back(vp_index[0]);
			vertexIndices.push_back(vp_index[1]);
			vertexIndices.push_back(vp_index[2]);

			uvIndices.push_back(vt_index[0]);
			uvIndices.push_back(vt_index[1]);
			uvIndices.push_back(vt_index[2]);

			normalIndices.push_back(vn_index[0]);
			normalIndices.push_back(vn_index[1]);
			normalIndices.push_back(vn_index[2]);

			if (iss >> token)
			{
				vertexIndices.push_back(vp_index[0]);
				vertexIndices.push_back(vp_index[2]);
				vertexIndices.push_back(std::stoi(token) - 1);

				uvIndices.push_back(vt_index[0]);
				uvIndices.push_back(vt_index[2]);
				uvIndices.push_back(std::stoi(token) - 1);

				normalIndices.push_back(vn_index[0]);
				normalIndices.push_back(vn_index[2]);
				normalIndices.push_back(std::stoi(token) - 1);
			}
		}
	}
	objFile.close();

	for (uint i = 0; i < vertexIndices.size(); i++)
	{
		uint vertexIndex = vertexIndices[i];
		uint uvIndex = uvIndices[i];
		uint normalIndex = normalIndices[i];

		Vec3 vertex = temp_vertices[vertexIndex];
		out_vertices.push_back(vertex);

		if (temp_uvs.size() > 0 && temp_normals.size() > 0)
		{
			TextureCoord texture = temp_uvs[uvIndex];
			Vec3 normal = temp_normals[normalIndex];

			out_uvs.push_back(texture);
			out_normals.push_back(normal);
		}
	}

	if (out_uvs.size() == 0)
	{
		for (uint i = 0; i < out_vertices.size(); i += 6)
		{
			out_uvs.push_back({ 0.0f, 0.0f });
			out_uvs.push_back({ 0.0f, 1.0f });
			out_uvs.push_back({ 1.0f, 1.0f });

			out_uvs.push_back({ 0.0f, 0.0f });
			out_uvs.push_back({ 1.0f, 1.0f });
			out_uvs.push_back({ 1.0f, 0.0f });
		}
	}

	if (out_normals.size() == 0)
	{
		for (uint i = 0; i < out_vertices.size(); i += 3)
		{
			Vec3 v0 = out_vertices[i];
			Vec3 v1 = out_vertices[i + 1];
			Vec3 v2 = out_vertices[i + 2];

			Vec3 edge1 = v1 - v0;
			Vec3 edge2 = v2 - v0;

			Vec3 normal = Vec3::cross(edge1, edge2);

			normal = Vec3::normalize(normal);

			out_normals.push_back(normal);
			out_normals.push_back(normal);
			out_normals.push_back(normal);
		}
	}

	indexVBO(out_vertices, out_uvs, out_normals);
	createBuffersAndArrays();
}

bool	getSimilarVertexIndex(PackedVertex &packed, std::map<PackedVertex, ushort> &VertexToOutIndex, ushort &result)
{
	std::map<PackedVertex, ushort>::iterator it = VertexToOutIndex.find(packed);
	if (it == VertexToOutIndex.end())
		return false;
	else
	{
		result = it->second;
		return true;
	}
}

void	Scop::indexVBO(std::vector<Vec3> &in_vertices, std::vector<TextureCoord> &in_uvs, std::vector<Vec3> &in_normals)
{
	std::map<PackedVertex, ushort> vertexToOutIndex;

	for (uint i = 0; i < in_vertices.size(); i++)
	{
		PackedVertex packed = { in_vertices[i], in_uvs[i], in_normals[i] };

		ushort index;
		bool found = getSimilarVertexIndex(packed, vertexToOutIndex, index);

		if (found)
			this->indices.push_back(index);
		else
		{
			this->vertex_postitions.push_back(in_vertices[i]);
			this->vertex_texcoords.push_back(in_uvs[i]);
			this->vertex_normals.push_back(in_normals[i]);

			ushort newIndex = static_cast<ushort>(this->vertex_postitions.size() - 1);
			this->indices.push_back(newIndex);
			vertexToOutIndex[packed] = newIndex;
		}
	}
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
