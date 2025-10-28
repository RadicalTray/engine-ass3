#version 430
#define MAX_BONE_INFLUENCE 4

in VS_OUT {
	vec3 FragPos;
	vec3 Normal;
	vec2 TexCoord;
} vsOut;

layout(location = 0) out vec4 FragColor;

layout(binding = 0) uniform UniformBuffer {
	mat4 projection;
	mat4 view;
	mat4 model;
	mat4 model_IT;
	vec4 viewPos;
	vec4 lightPos;
	vec4 lightClr;
	vec4 ambientClr;
	float ambientStr;
};

layout(location = 0) uniform sampler2D diffuse_texture;
layout(location = 1) uniform sampler2D specular_texture;
layout(location = 2) uniform sampler2D normal_texture;
layout(location = 3) uniform sampler2D height_texture;

// TODO:
//	Normal texture
//	Specular texture
//	Height texture (tf is a height texture, isn't it for tesselation?)
void main() {
	vec4 color = texture(diffuse_texture, vsOut.TexCoord);
	vec3 ambient = ambientStr * ambientClr.xyz;

	vec3 norm = normalize(vsOut.Normal);
	vec3 lightDir = normalize(lightPos.xyz - vsOut.FragPos);
	vec3 diffuse = lightClr.xyz * max(dot(norm, lightDir), 0.0);

	float specularStr = lightPos.w;
	vec3 viewDir = normalize(viewPos.xyz - vsOut.FragPos);
	vec3 reflectDir = reflect(-lightDir, norm);
	vec3 specular = specularStr * pow(max(dot(viewDir, reflectDir), 0.0), 32) * lightClr.xyz;

	FragColor = vec4(ambient + diffuse + specular, 1.0f) * color;
}
