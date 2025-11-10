#version 430

layout(location = 0) in vec3 aTexCoord;

layout(location = 0) out vec4 FragColor;

layout(location = 0) uniform samplerCube cubemapTexture;

void main() {
	FragColor = texture(cubemapTexture, aTexCoord);
}
