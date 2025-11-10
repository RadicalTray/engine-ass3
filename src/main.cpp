#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <algorithm>
#include <cmath>
#include <chrono>
#include <thread>
#include <iostream>
#include <vector>
#include <array>

#include <utils.hpp>
#include <model.hpp>
#include <animation.hpp>
#include <animator.hpp>

mat4 getView(vec3 model_pos, vec3 front, vec3 up, bool cam_zero);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void windowSizeCallback(GLFWwindow* window, int width, int height);

struct Mouse {
	double last_xpos;
	double last_ypos;
	double sens;
	float yaw;
	float pitch;
};

struct UniformBuffer {
	mat4 model;
	mat4 model_it;
	mat4 view;
	mat4 projection;
	vec4 view_pos;
	vec4 light_pos;
	vec4 light_clr;
	vec4 ambient_clr;
	float ambient_str;
};

struct View {
	vec3 pos;
	vec3 front;
	vec3 up;
	float fov;
	float speed;
};

struct Keys {
	bool left_click;
	bool right_click;
	bool w;
	bool s;
	bool a;
	bool d;
	bool e;
	bool q;
	bool space;
	bool shift;
	bool tab;
};

enum PlayerState {
	IDLE,
	SWIM,
	WALK,
};

struct State {
	UniformBuffer ub;
	Mouse mouse;
	View view;
	ivec2 scr_res;
	int faces;
	float dt;
	Keys keys;
	PlayerState player_state;

	static State init(GLFWwindow *const window) {
		State state = {};
		state.player_state = IDLE;
		state.faces = 4;
		state.mouse = {
			.sens = 0.1f,
			.yaw = -90.0f,
		};
		state.view = {
			.pos = vec3(0.0f, 0.0f, -1.0f),
			.front = vec3(0.0f, 0.0f, -1.0f),
			.up = vec3(0.0f, 1.0f, 0.0f),
			.fov = 90.0f,
			.speed = 0.004f,
		};
		state.ub = {
			.light_pos = vec4(0.0f, 100.0f, 1.0f, 0.2f),
			.light_clr = vec4(1.0f),
			.ambient_clr = vec4(1.0f),
			.ambient_str = 0.5f,
		};
		glfwGetWindowSize(window, &state.scr_res.x, &state.scr_res.y);
		glfwGetCursorPos(window, &state.mouse.last_xpos, &state.mouse.last_ypos);
		state.updateViewProj(vec3(0.0f));
		state.updateModel(vec3(0.0f), vec3(1.0f), vec2(0.0f));
		return state;
	}

	void uploadUB(uint ubo) const {
		glNamedBufferSubData(ubo, 0, sizeof(UniformBuffer), &this->ub);
	}

	void updateViewProj(vec3 pos) {
		// pinned cam
		this->ub.view = getView(pos, this->view.front, this->view.up, false);
		// free cam
		// this->ub.view = glm::lookAt(this->view.pos, this->view.pos + this->view.front, this->view.up);

		this->ub.projection = glm::perspective(glm::radians(this->view.fov), (float)this->scr_res.x / (float)this->scr_res.y, 0.1f, 100.0f);

		this->ub.view_pos = vec4(this->view.pos, 0.0f);
	}

	void uploadModelViewProj(uint ubo) {
		glNamedBufferSubData(ubo, 0, 4*sizeof(mat4) + sizeof(vec4), &this->ub);
	}

	void uploadViewProj(uint ubo) {
		glNamedBufferSubData(ubo, offsetof(UniformBuffer, view), 2*sizeof(mat4) + sizeof(vec4), &this->ub);
	}

	void updateModel(vec3 pos, vec3 scale, vec2 front) {
		const vec3 up = vec3(0.0f, 1.0f, 0.0f);

		// xz
		vec2 u = vec2(1.0f, 0.0f);
		vec2 v = front;
		float angle = std::atan2(u.x*v.x + u.y*v.y, u.x*v.y - u.y*v.x);

		mat4 model(1.0f);
		model = glm::scale(model, scale);
		model = glm::translate(model, pos);
		model = glm::rotate(model, angle, up);

		this->ub.model = model;
		this->ub.model_it = glm::transpose(glm::inverse(model));
	}

	void uploadModel(uint ubo) const {
		glNamedBufferSubData(ubo, offsetof(UniformBuffer, model), 2*sizeof(mat4), &this->ub);
	}
};

struct CubeMap {
	std::vector<vec3> vertices;
	std::vector<uint> indices;
	uint vao;
	uint vbo;
	uint ebo;
	uint shader;
	uint tex;

	static CubeMap init() {
		CubeMap cube_map = {};

		glCreateVertexArrays(1, &cube_map.vao);
		glEnableVertexArrayAttrib(cube_map.vao,  0);
		glVertexArrayAttribFormat(cube_map.vao,  0, 3, GL_FLOAT, GL_FALSE, 0);
		glVertexArrayAttribBinding(cube_map.vao, 0, 0);

		cube_map.shader = createShader("./shaders/cube_map.vert", "./shaders/cube_map.frag");
		glProgramUniform1i(cube_map.shader, 0, 0);

		const char *paths[6] = {
			"assets/envmap_miramar/miramar_ft.tga",
			"assets/envmap_miramar/miramar_bk.tga",
			"assets/envmap_miramar/miramar_up.tga",
			"assets/envmap_miramar/miramar_dn.tga",
			"assets/envmap_miramar/miramar_rt.tga",
			"assets/envmap_miramar/miramar_lf.tga",
		};

		glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &cube_map.tex);
		glTextureParameteri(cube_map.tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(cube_map.tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(cube_map.tex, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTextureParameteri(cube_map.tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(cube_map.tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		bool texture_allocated = false;

		stbi_set_flip_vertically_on_load(false);
		for (int i = 0; i < 6; i++) {
			int width, height, n_channels;
			uchar *data = stbi_load(paths[i], &width, &height, &n_channels, 3);
			if (data) {
				if (!texture_allocated) {
					glTextureStorage2D(cube_map.tex, 1, GL_RGB8, width, height);
					texture_allocated = true;
				}
				glTextureSubImage3D(cube_map.tex, 0, 0, 0, i, width, height, 1, GL_RGB, GL_UNSIGNED_BYTE, data);
			} else {
				std::cerr << "stbi(error): " << stbi_failure_reason() << " (" << paths[i] << ")" << std::endl;
			}
			stbi_image_free(data);
		}

		uint flags = 0;
		flags |= aiProcess_Triangulate;

		const char *path = "assets/cube.obj";
		std::cerr << "assimp(info): loading " << path << std::endl;
		Assimp::Importer imp;
		const aiScene *scene = imp.ReadFile(path, flags);
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			std::cerr << "assimp(error): " << imp.GetErrorString() << std::endl;
			return {}; // TODO: handle error
		}
		cube_map.processNode(scene->mRootNode, scene);

		glCreateBuffers(2, &cube_map.vbo);
		glNamedBufferData(cube_map.vbo, cube_map.vertices.size() * sizeof(vec3), cube_map.vertices.data(), GL_STATIC_DRAW);
		glNamedBufferData(cube_map.ebo, cube_map.indices.size() * sizeof(uint), cube_map.indices.data(), GL_STATIC_DRAW);

		return cube_map;
	}

	void processNode(aiNode* node, const aiScene* scene) {
		for (uint i = 0; i < node->mNumMeshes; i++) {
			aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];

			this->vertices.reserve(this->vertices.size() + mesh->mNumVertices);
			for (uint i = 0; i < mesh->mNumVertices; i++) {
				this->vertices.push_back(vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z));
			}

			this->indices.reserve(this->indices.size() + mesh->mNumFaces * 3);
			for (uint i = 0; i < mesh->mNumFaces; i++) {
				aiFace face = mesh->mFaces[i];
				for (uint j = 0; j < face.mNumIndices; j++) {
					this->indices.push_back(face.mIndices[j]);
				}
			}
		}
		for (uint i = 0; i < node->mNumChildren; i++) {
			processNode(node->mChildren[i], scene);
		}
	}

	void draw() const {
		int old_cull_face_mode;
		glGetIntegerv(GL_CULL_FACE_MODE, &old_cull_face_mode);
		int old_depth_func;
		glGetIntegerv(GL_DEPTH_FUNC, &old_depth_func);

		glCullFace(GL_FRONT);
		glDepthFunc(GL_LEQUAL);

		glUseProgram(this->shader);
		glBindVertexArray(this->vao);

		// uniform has already been set to texture 0 in init()
		glBindTextureUnit(0, this->tex);

		glVertexArrayVertexBuffer(this->vao, 0, this->vbo, 0, sizeof(vec3));
		glVertexArrayElementBuffer(this->vao, this->ebo);
		glDrawElements(GL_TRIANGLES, static_cast<uint>(this->indices.size()), GL_UNSIGNED_INT, 0);

		glCullFace(old_cull_face_mode);
		glDepthFunc(old_depth_func);
	}
};

const float flr = 0.0f;
const float gravity = 0.0002f;

int main() {
	GLFWwindow *window = init();
	State state = State::init(window);
	glfwSetWindowUserPointer(window, reinterpret_cast<void *>(&state));
	glfwSetKeyCallback(window, keyCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwSetCursorPosCallback(window, cursorPosCallback);
	glfwSetScrollCallback(window, scrollCallback);
	glfwSetWindowSizeCallback(window, windowSizeCallback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Initialize buffers
	std::array<uint, 1> va{};
	std::array<uint, 1> b{};
	glCreateVertexArrays(va.size(), va.data());
        glCreateBuffers(b.size(), b.data());

	const uint vao = va[0];
	const uint ubo = b[0];

	Vertex::setupVAO(vao);

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);
	glNamedBufferData(ubo, sizeof(UniformBuffer), &state.ub, GL_DYNAMIC_DRAW);

	// Initialize shaders
	const uint model_vert_shader = createShader("./shaders/model.vert", "./shaders/model_vert.frag");
	const uint model_shader = createShader("./shaders/model.vert", "./shaders/model.frag");
	const uint model_anim_shader = createShader("./shaders/model_anim.vert", "./shaders/model.frag");
	const uint model_plain_shader = createShader("./shaders/model.vert", "./shaders/model_plain.frag");
	const uint model_plain_anim_shader = createShader("./shaders/model_anim.vert", "./shaders/model_plain.frag");

	// TODO: move this inside model
	const int model_anim_shader_bone_matrices = glGetUniformLocation(model_plain_anim_shader, "boneMatrices");

	// Model model = Model::init("./vampire/dancing_vampire.dae", false);
	Model model = Model::init("./assets/Dancing Twerk.dae", vao, model_plain_anim_shader);
	model.hitbox = { .min = vec3(0.0f), .max = vec3(0.4f) };

	// auto dance_anim = Animation::init("./vampire/dancing_vampire.dae", model.bone_info_map);
	auto dance_anim = Animation::init("./assets/Dancing Twerk.dae", model.bone_info_map);
	auto swim_anim = Animation::init("./assets/Swimming.dae", model.bone_info_map);
	auto walk_anim = Animation::init("./assets/Walking.dae", model.bone_info_map);
	auto animator = Animator::init(&dance_anim);
	assert(animator.bone_matrices.size() <= MAX_BONE_MATRICES);

	Model tower = Model::init("./assets/fantasy_tower/scene.gltf", vao, model_shader);
	Model map = Model::init("./assets/low_poly_island/scene.gltf", vao, model_shader);
	// Model cat = Model::init("./assets/cat_low_poly.glb", vao, model_plain_shader);

	// TODO: store a ptr/handle to Model instead of having multiple copies
	//  Potential solution: DrawContext{ vao, shader, drawfn }
	std::vector<Model> objs;

	CubeMap cube_map = CubeMap::init();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	// glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	auto start = chrono::steady_clock::now();
	while (!glfwWindowShouldClose(window)) {
		auto now = chrono::steady_clock::now();
		state.dt = chrono::duration_cast<chrono::microseconds>(now - start).count();
		start = now;

		{ // process
			glfwPollEvents();
			const float dt_ms = state.dt/1000.0f;

			// Floating cam
			// if (state.keys.w) state.view.pos += state.view.front * state.view.speed * dt_ms;
			// if (state.keys.s) state.view.pos -= state.view.front * state.view.speed * dt_ms;
			// if (state.keys.a) state.view.pos -= glm::normalize(glm::cross(state.view.front, state.view.up)) * state.view.speed * dt_ms;
			// if (state.keys.d) state.view.pos += glm::normalize(glm::cross(state.view.front, state.view.up)) * state.view.speed * dt_ms;
			// if (state.keys.space) state.view.pos += state.view.up * state.view.speed * dt_ms;
			// if (state.keys.shift) state.view.pos -= state.view.up * state.view.speed * dt_ms;

			vec3 move_front = state.view.front * state.view.speed * dt_ms;
			move_front.y = 0.0f;
			vec3 move_side = glm::normalize(glm::cross(state.view.front, state.view.up)) * state.view.speed * dt_ms;
			vec3 move_vert = state.view.up * 0.01f;

			model.velocity.x = 0;
			model.velocity.y -= gravity * dt_ms;
			model.velocity.z = 0;

			if (state.keys.w)     model.velocity   += move_front;
			if (state.keys.s)     model.velocity   -= move_front;
			if (state.keys.a)     model.velocity   -= move_side;
			if (state.keys.d)     model.velocity   += move_side;
			if (state.keys.space) model.velocity.y += 1.35f*gravity*dt_ms;
			// if (state.keys.shift) model.velocity -= move_vert;

			vec3 new_pos = model.pos + model.velocity;

			// TODO:
			// "physics"
			if (new_pos.y < flr) {
				new_pos.y = flr;
				model.velocity.y = 0.0f;
			}

			for (usize i = 0; i < objs.size(); i++) {
				model.detectObj(new_pos, objs[i]);
			}
			model.pos = new_pos;

			switch (state.player_state) {
			case IDLE:
				if (model.velocity.x != 0) {
					if (new_pos.y == flr) {
						state.player_state = WALK;
						animator.playAnimation(&walk_anim);
					} else {
						state.player_state = SWIM;
						animator.playAnimation(&swim_anim);
					}
				}
				break;
			case SWIM:
				if (model.velocity.x == 0) {
					state.player_state = IDLE;
					animator.playAnimation(&dance_anim);
				} else if (model.pos.y == flr) {
					state.player_state = WALK;
					animator.playAnimation(&walk_anim);
				}
				break;
			case WALK:
				if (model.velocity.x == 0) {
					state.player_state = IDLE;
					animator.playAnimation(&dance_anim);
				} else if (model.pos.y > flr) {
					state.player_state = SWIM;
					animator.playAnimation(&swim_anim);
				}
				break;
			}

			animator.updateAnimation(state.dt/1000000.0f);
		}
		{ // render
			glClearColor(0.0f, 0.0f, 0.0f, 1.00f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// render player model
			const auto& transforms = animator.bone_matrices;
			glProgramUniformMatrix4fv(model_plain_anim_shader, model_anim_shader_bone_matrices, transforms.size(), false, glm::value_ptr(transforms[0]));

			state.updateModel(model.pos, vec3(1.0f), vec2(state.view.front.x, state.view.front.z));
			state.updateViewProj(model.pos);
			state.uploadModelViewProj(ubo);
			model.draw();

			// render objects
			for (auto o : objs) {
				state.updateModel(o.pos, vec3(1.0f), vec2(0.0f));
				state.uploadModel(ubo);
				o.draw();
			}
			state.updateModel(vec3(0.0f, -2.0f, 0.0f), vec3(4.0f, 1.0f, 4.0f), vec2(0.0f));
			state.uploadModel(ubo);
			map.draw();
			state.updateModel(vec3(0.0f, 1.0f, 0.0f), vec3(10.0f), vec2(0.0f));
			state.uploadModel(ubo);
			tower.draw();

			// render cube map
			mat4 view = getView(vec3(0.0), state.view.front, state.view.up, true);
			glNamedBufferSubData(ubo, offsetof(UniformBuffer, view), sizeof(mat4), glm::value_ptr(view));
			cube_map.draw();
		}
		glfwSwapBuffers(window);
	}

	// TODO: free stuff
	// glDeleteVertexArrays(, );
	// glDeleteBuffers(, );
	// glDeleteProgram(model_plain_shader);
	// glDeleteProgram(model_plain_anim_shader);

	deinit(&window);
	return 0;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	State *state = reinterpret_cast<State*>(glfwGetWindowUserPointer(window));
	switch (key) {
	case GLFW_KEY_W:
		switch (action) {
		case GLFW_PRESS:
			state->keys.w = true;
			break;
		case GLFW_RELEASE:
			state->keys.w = false;
			break;
		}
		break;
	case GLFW_KEY_S:
		switch (action) {
		case GLFW_PRESS:
			state->keys.s = true;
			break;
		case GLFW_RELEASE:
			state->keys.s = false;
			break;
		}
		break;
	case GLFW_KEY_A:
		switch (action) {
		case GLFW_PRESS:
			state->keys.a = true;
			break;
		case GLFW_RELEASE:
			state->keys.a = false;
			break;
		}
		break;
	case GLFW_KEY_D:
		switch (action) {
		case GLFW_PRESS:
			state->keys.d = true;
			break;
		case GLFW_RELEASE:
			state->keys.d = false;
			break;
		}
		break;
	case GLFW_KEY_E:
		switch (action) {
		case GLFW_PRESS:
			state->keys.e = true;
			break;
		case GLFW_RELEASE:
			state->keys.e = false;
			break;
		}
		break;
	case GLFW_KEY_Q:
		switch (action) {
		case GLFW_PRESS:
			state->keys.q = true;
			break;
		case GLFW_RELEASE:
			state->keys.q = false;
			break;
		}
		break;
	case GLFW_KEY_SPACE:
		switch (action) {
		case GLFW_PRESS:
			state->keys.space = true;
			break;
		case GLFW_RELEASE:
			state->keys.space = false;
			break;
		}
		break;
	case GLFW_KEY_LEFT_SHIFT:
		switch (action) {
		case GLFW_PRESS:
			state->keys.shift = true;
			break;
		case GLFW_RELEASE:
			state->keys.shift = false;
			break;
		}
		break;
	case GLFW_KEY_TAB:
		switch (action) {
		case GLFW_PRESS:
			state->keys.tab = true;
			break;
		case GLFW_RELEASE:
			state->keys.tab = false;
			break;
		}
		break;
	}
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	State *state = reinterpret_cast<State*>(glfwGetWindowUserPointer(window));
	switch (button) {
	case GLFW_MOUSE_BUTTON_LEFT:
		switch (action) {
		case GLFW_PRESS:
			state->keys.left_click = true;
			break;
		case GLFW_RELEASE:
			state->keys.left_click = false;
			break;
		}
		break;
	case GLFW_MOUSE_BUTTON_RIGHT:
		switch (action) {
		case GLFW_PRESS:
			state->keys.right_click = true;
			break;
		case GLFW_RELEASE:
			state->keys.right_click = false;
			break;
		}
		break;
	}
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
	State *state = reinterpret_cast<State*>(glfwGetWindowUserPointer(window));
	double xoffset =  (xpos - state->mouse.last_xpos) * state->mouse.sens;
	double yoffset = -(ypos - state->mouse.last_ypos) * state->mouse.sens;
	state->mouse.last_xpos = xpos;
	state->mouse.last_ypos = ypos;

	state->mouse.yaw = state->mouse.yaw + xoffset;
	state->mouse.pitch = std::clamp(state->mouse.pitch + yoffset, -89.0d, 89.0d);
	state->view.front = glm::normalize(vec3(
		std::cos(glm::radians(state->mouse.yaw)) * std::cos(glm::radians(state->mouse.pitch)),
		std::sin(glm::radians(state->mouse.pitch)),
		std::sin(glm::radians(state->mouse.yaw)) * std::cos(glm::radians(state->mouse.pitch))
	));
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	// State *state = reinterpret_cast<State*>(glfwGetWindowUserPointer(window));
}

void windowSizeCallback(GLFWwindow* window, int width, int height) {
	State *state = reinterpret_cast<State*>(glfwGetWindowUserPointer(window));
	state->scr_res = ivec2(width, height);
}

mat4 getView(vec3 model_pos, vec3 front, vec3 up, bool cam_zero) {
	vec3 radius = 2.0f * front;
	if (!cam_zero) {
		model_pos.y += 1.3f;
		return glm::lookAt(model_pos - radius, model_pos, up);
	} else {
		return glm::lookAt(vec3(0.0), radius, up);
	}
}
