#version 460 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

out vec2 TexCoord;
out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
	gl_Position = projection * view * model * vec4(inPosition, 1.0);
	TexCoord = inTexCoord;
	FragPos = vec3(model * vec4(inPosition, 1.0));
	Normal = mat3(transpose(inverse(model))) * inNormal;
}
