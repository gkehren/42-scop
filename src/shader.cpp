#include "../include/Scop.hpp"

// Function to load a shader source file
static std::string	loadShaderSource(const char* filename)
{
	std::ifstream	file(filename);
	if (!file.is_open())
	{
		std::cerr << "Failed to open " << filename << std::endl;
		return "";
	}

	std::string source;
	std::string line;
	while (std::getline(file, line))
		source += line + "\n";

	file.close();
	return source;
}

// Function to compile a shader
static uint	compileShader(GLenum shaderType, const char* source)
{
	uint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		GLchar infoLog[512];
		glGetShaderInfoLog(shader, sizeof(infoLog), NULL, infoLog);
		std::cerr << "Failed to compile shader:\n" << infoLog << std::endl;
		return 0;
	}

	return shader;
}

// Function to create a shader program
static uint	createShaderProgram(const char* vertexShaderSource, const char* fragmentShaderSource)
{
	uint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
	uint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

	uint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	GLint success;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		GLchar infoLog[512];
		glGetProgramInfoLog(shaderProgram, sizeof(infoLog), NULL, infoLog);
		std::cerr << "Failed to link shader program:\n" << infoLog << std::endl;
		return 0;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

void	Scop::loadShader()
{
	std::string vertexShaderSource = loadShaderSource("/home/gkehren/42-scop/ressources/shaders/vertex.glsl");
	std::string fragmentShaderSource = loadShaderSource("/home/gkehren/42-scop/ressources/shaders/fragment.glsl");

	this->shaderProgram = createShaderProgram(vertexShaderSource.c_str(), fragmentShaderSource.c_str());

	if (shaderProgram == 0)
	{
		std::cerr << "Error: could not create shader program" << std::endl;
		throw std::runtime_error("Error: could not create shader program");
	}

	glUseProgram(this->shaderProgram);
}
