#version 430
#define MAX_BONE_MATRICES 128
#define MAX_BONE_INFLUENCE 4

// TODO: take boneIDs and weights out of model.vert
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;
layout(location = 5) in int aBoneIDs[MAX_BONE_INFLUENCE];
layout(location = 6) in float aWeights[MAX_BONE_INFLUENCE];
layout(location = 7) in vec4 aColor;

out VS_OUT {
	vec3 FragPos;
	vec3 Normal;
	vec2 TexCoord;
	vec4 FragColor;
} vsOut;

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

void main() {
	vec4 fragPos = model * vec4(aPos, 1.0f);
	vec4 pos = projection * view * fragPos;
	vec3 normal = aNormal;
	vec2 texCoord = aTexCoord;

	gl_Position = pos;
	vsOut.FragPos = fragPos.xyz / fragPos.w;
	vsOut.Normal = mat3(model_IT) * normal;
	vsOut.TexCoord = texCoord;
	vsOut.FragColor = aColor;
}
