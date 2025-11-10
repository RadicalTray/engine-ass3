#version 430

layout(location = 0) in vec3 aPos;

layout(location = 0) out vec3 TexCoord;

layout(binding = 0) uniform UniformBuffer {
	mat4 model;
	mat4 model_IT;
	mat4 view;
	mat4 projection;
	vec4 viewPos;
	vec4 lightPos;
	vec4 lightClr;
	vec4 ambientClr;
	float ambientStr;
};

layout(location = 0) uniform samplerCube cubemapTexture; // unused

void main() {
	vec4 VP_Pos = projection * view * vec4(aPos, 1.0f);
	gl_Position = VP_Pos.xyww;
	TexCoord = aPos;
}
