#version 430
#define MAX_BONE_MATRICES 128
#define MAX_BONE_INFLUENCE 4

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;
layout(location = 5) in int aBoneIDs[MAX_BONE_INFLUENCE];
layout(location = 6) in float aWeights[MAX_BONE_INFLUENCE];

out VS_OUT {
	vec3 FragPos;
	vec3 Normal;
	vec2 TexCoord;
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
uniform mat4 boneMatrices[MAX_BONE_MATRICES];

void main() {
	vec4 totalPos = vec4(0.0f);
	for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
		// NOTE: this should be unreachable
		// if (aBoneIDs[i] < 0) {
		// 	continue;
		// }
		// NOTE: aBoneIDs WILL ALWAYS SUCCEED in comparing??? WHAT, except < 0 tho that didn't go in
		// if (aBoneIDs[i] > 20000) {
		// 	totalPos = vec4(aPos, 1.0f);
		// 	break;
		// }

		totalPos += aWeights[i] * boneMatrices[aBoneIDs[i]] * vec4(aPos, 1.0f);

		// TODO: calculate normal
		// vec3 localNormal = mat3(boneMatrices[aBoneIDs[i]]) * aNormal;
	}

	vec4 fragPos = totalPos;
	// vec4 fragPos = model * totalPos;
	vec4 pos = projection * view * fragPos;
	vec3 normal = aNormal;
	vec2 texCoord = aTexCoord;

	gl_Position = pos;
	vsOut.FragPos = fragPos.xyz / fragPos.w;
	vsOut.Normal = mat3(model_IT) * normal;
	vsOut.TexCoord = texCoord;
}
