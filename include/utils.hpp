#pragma once

#include <glad/gl.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include <stb/stb_image.h>

#include <fstream>
#include <sstream>
#include <string>

#define MAX_BONE_INFLUENCE 4

using glm::mat4, glm::vec2, glm::vec3, glm::vec4, glm::uvec2, glm::ivec2;
namespace chrono = std::chrono;

typedef unsigned char uchar;
typedef uint32_t uint;
typedef size_t usize;

struct Vertex {
	vec3 pos;
	vec3 norm;
	vec2 tex;
	vec3 tan;
	vec3 bitan;
	std::array<int, MAX_BONE_INFLUENCE> bone_ids;
	std::array<float, MAX_BONE_INFLUENCE> weights;

	static void setupVAO(uint vao) {
		glEnableVertexArrayAttrib(vao,  0);
		glVertexArrayAttribFormat(vao,  0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, pos));
		glVertexArrayAttribBinding(vao, 0, 0);

		glEnableVertexArrayAttrib(vao,  1);
		glVertexArrayAttribFormat(vao,  1, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, norm));
		glVertexArrayAttribBinding(vao, 1, 0);

		glEnableVertexArrayAttrib(vao,  2);
		glVertexArrayAttribFormat(vao,  2, 2, GL_FLOAT, GL_FALSE, offsetof(Vertex, tex));
		glVertexArrayAttribBinding(vao, 2, 0);

		glEnableVertexArrayAttrib(vao,  3);
		glVertexArrayAttribFormat(vao,  3, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, tan));
		glVertexArrayAttribBinding(vao, 3, 0);

		glEnableVertexArrayAttrib(vao,  4);
		glVertexArrayAttribFormat(vao,  4, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, bitan));
		glVertexArrayAttribBinding(vao, 4, 0);

		glEnableVertexArrayAttrib(vao,  5);
		glVertexArrayAttribFormat(vao,  5, MAX_BONE_INFLUENCE, GL_INT, GL_FALSE, offsetof(Vertex, bone_ids));
		glVertexArrayAttribBinding(vao, 5, 0);

		glEnableVertexArrayAttrib(vao,  6);
		glVertexArrayAttribFormat(vao,  6, MAX_BONE_INFLUENCE, GL_FLOAT, GL_FALSE, offsetof(Vertex, weights));
		glVertexArrayAttribBinding(vao, 6, 0);
	}
};

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
void GLAPIENTRY debugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
std::string readFile(const char *const filepath);

GLFWwindow* init() {
	glfwInit();

	GLFWwindow* window = glfwCreateWindow(1600, 900, "uwu", NULL, NULL);
	if (window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		exit(-1);
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	int version = gladLoadGL(glfwGetProcAddress);
	if (version == 0) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		exit(-1);
	}
	std::cout << "GL " << GLAD_VERSION_MAJOR(version) << "." << GLAD_VERSION_MINOR(version) << std::endl;

	// first window doesn't trigger framebufferSizeCallback
	// or has the wrong viewport for some reason
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	framebufferSizeCallback(window, width, height);

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(debugMessageCallback, 0);

	return window;
}

void deinit(GLFWwindow** window) {
	glfwTerminate();
	*window = nullptr;
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

void GLAPIENTRY
debugMessageCallback(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam
) {
	// WARN: assuming message is always NUL terminated. (can be checked with `length`)
	std::string source_str, type_str, severity_str;
	switch (source) {
		case GL_DEBUG_SOURCE_API:		source_str = "api"; break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:	source_str = "window system"; break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER:	source_str = "shader compiler"; break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:	source_str = "third party"; break;
		case GL_DEBUG_SOURCE_APPLICATION:	source_str = "application"; break;
		case GL_DEBUG_SOURCE_OTHER:		source_str = "other"; break;
	}
	switch (type) {
		case GL_DEBUG_TYPE_ERROR:			type_str = "error"; break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:		type_str = "deprecated"; break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:		type_str = "undefined"; break; 
		case GL_DEBUG_TYPE_PORTABILITY:			type_str = "portability"; break;
		case GL_DEBUG_TYPE_PERFORMANCE:			type_str = "performance"; break;
		case GL_DEBUG_TYPE_MARKER:			type_str = "marker"; break;
		case GL_DEBUG_TYPE_PUSH_GROUP:			type_str = "push group"; break;
		case GL_DEBUG_TYPE_POP_GROUP:			type_str = "pop group"; break;
		case GL_DEBUG_TYPE_OTHER:			type_str = "other"; break;
	}
	switch (severity) {
		case GL_DEBUG_SEVERITY_HIGH:		severity_str = "high"; break;
		case GL_DEBUG_SEVERITY_MEDIUM:		severity_str = "medium"; break;
		case GL_DEBUG_SEVERITY_LOW:		severity_str = "low"; break;
		case GL_DEBUG_SEVERITY_NOTIFICATION:	severity_str = "notification"; break;
	}
	// std::cerr << "GL(" << type_str << ", " << source_str << ", " << severity_str << ", " << id << "): " << message << std::endl;
	std::cerr << "GL(" << type_str << "): " << message << std::endl;
}

uint texture2DFromFile(const char* filepath, int levels) {
	stbi_set_flip_vertically_on_load(true); // opengl/glfw dum dum

	uint tex;
	glCreateTextures(GL_TEXTURE_2D, 1, &tex);

	int width, height, nrChannels;
	uchar *data = stbi_load(filepath, &width, &height, &nrChannels, 0);
	if (data) {
		GLenum internalformat = GL_R8, format = GL_RED;
		switch (nrChannels) {
		case 1:
			internalformat = GL_R8;
			format = GL_RED;
			break;
		case 2:
			internalformat = GL_RG8;
			format = GL_RG;
			break;
		case 3:
			internalformat = GL_RGB8;
			format = GL_RGB;
			break;
		case 4:
			internalformat = GL_RGBA8;
			format = GL_RGBA;
			break;
		default:
			std::cerr << "error: " << filepath << "wtf is this format?" << std::endl;
			break;
		}
		glTextureStorage2D(tex, levels, internalformat, width, height);
		glTextureSubImage2D(tex, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, data);
		glGenerateTextureMipmap(tex);
	} else {
		std::cerr << "stbi(error): " << stbi_failure_reason() << " (" << filepath << ")" << std::endl;
	}
	stbi_image_free(data);

	return tex;
}

uint createShader(const char *const vert_filename, const char *const frag_filename) {
	const std::string vert_src = readFile(vert_filename), frag_src = readFile(frag_filename);
	const char *vert_src_c = vert_src.data(), *frag_src_c = frag_src.data();

	uint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vert_src_c, NULL);
	glCompileShader(vertex_shader);

	uint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &frag_src_c, NULL);
	glCompileShader(fragment_shader);

	uint shader = glCreateProgram();
	glAttachShader(shader, vertex_shader);
	glAttachShader(shader, fragment_shader);
	glLinkProgram(shader);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	return shader;
}

std::string readFile(const char *const filepath) {
	std::ifstream file;
	file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	file.open(filepath);
	std::stringstream stream;
	stream << file.rdbuf();
	file.close();
	return stream.str();
}
