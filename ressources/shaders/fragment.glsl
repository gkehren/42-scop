#version 330 core

in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec3 objectColor;

uniform sampler2D textureSampler; // Texture sampler
uniform bool showLight; // Show light
uniform float transitionFactor; // Transition factor between texture and gradient
uniform bool showGradient; // Show gradient instead of texture
uniform vec3 gradientStartColor; // Gradient start color
uniform vec3 gradientEndColor; // Gradient end color

void main() {
	vec3 result = vec3(0.0);

	if (showGradient) {
		float gradientFactor = (FragPos.y + 1.0) / 2.0;
		result = mix(gradientStartColor, gradientEndColor, gradientFactor);
	} else {
		vec3 textureColor = texture(textureSampler, TexCoord).rgb;
		result = mix(objectColor, textureColor, transitionFactor);
	}

	if (showLight) {
		// Calculate lighting
		vec3 norm = normalize(Normal);
		vec3 lightDir = normalize(lightPos - FragPos);

		// Diffuse shading
		float diff = max(dot(norm, lightDir), 0.0);

		// Specular shading
		vec3 viewDir = normalize(viewPos - FragPos);
		vec3 reflectDir = reflect(-lightDir, norm);
		float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);

		// Combine results
		vec3 ambient = 0.1 * objectColor;
		vec3 diffuse = diff * lightColor;
		vec3 specular = spec * lightColor;
		result = (ambient + diffuse + specular) * result;
	}

	FragColor = vec4(result, 1.0);
}
