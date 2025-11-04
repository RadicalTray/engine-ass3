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

namespace chrono = std::chrono;

const float flr = -0.0f;
const float gravity = 0.0002f;

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

struct State {
	UniformBuffer ub;
	Mouse mouse;
	View view;
	ivec2 scr_res;
	int faces;
	float dt;
	Keys keys;

	static State init(GLFWwindow *const window) {
		State state = {};
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
			.light_pos = vec4(1.2f, 10.0f, 2.0f, 0.2f),
			.light_clr = vec4(1.0f),
			.ambient_clr = vec4(1.0f),
			.ambient_str = 0.5f,
		};
		glfwGetWindowSize(window, &state.scr_res.x, &state.scr_res.y);
		glfwGetCursorPos(window, &state.mouse.last_xpos, &state.mouse.last_ypos);
		state.updateUB(vec3(0.0f));
		state.updateModel(vec3(0.0f), vec3(1.0f), vec2(0.0f));
		return state;
	}

	// does not update Model matrices
	void updateUB(vec3 pos) {
		this->ub.projection = glm::perspective(glm::radians(this->view.fov), (float)this->scr_res.x / (float)this->scr_res.y, 0.1f, 100.0f);

		// pinned cam
		pos.y += 1.0f;
		this->ub.view = glm::lookAt(pos - 1.2f * this->view.front, pos, this->view.up);
		// free cam
		// this->ub.view = glm::lookAt(this->view.pos, this->view.pos + this->view.front, this->view.up);

		this->ub.view_pos = vec4(this->view.pos, 0.0f);
	}

	// maybe we should consider merging this with updateXX lol
	//
	// NOTE: This also uploads model
	void uploadUB(uint ubo) const {
		glNamedBufferSubData(ubo, 0, sizeof(UniformBuffer), &this->ub);
	}

	void updateModel(vec3 pos, vec3 scale, vec2 front) {
		const vec3 up = vec3(0.0f, 1.0f, 0.0f);

		// xz
		vec2 u = vec2(1.0f, 0.0f);
		vec2 v = front;
		float angle = std::atan2(u.x*v.x + u.y*v.y, u.x*v.y - u.y*v.x);

		mat4 model(1.0f);
		model = glm::scale    (model, scale);
		model = glm::translate(model, pos);
		model = glm::rotate   (model, angle, up);

		this->ub.model = model;
		this->ub.model_it = glm::transpose(glm::inverse(model));
	}

	void uploadModel(uint ubo) const {
		glNamedBufferSubData(ubo, offsetof(UniformBuffer, model), 2*sizeof(mat4), &this->ub);
	}
};

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void windowSizeCallback(GLFWwindow* window, int width, int height);

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
	const uint model_shader = createShader("./shaders/model.vert", "./shaders/model.frag");
	const uint model_anim_shader = createShader("./shaders/model_anim.vert", "./shaders/model_plain.frag");
	int model_anim_shader_bone_matrices = glGetUniformLocation(model_anim_shader, "boneMatrices");

	// Model model = Model::init("./vampire/dancing_vampire.dae", false);
	Model model = Model::init("./Dancing Twerk.dae", false);
	model.hitbox = { .min = vec3(0.0f), .max = vec3(0.4f) };

	// auto danceAnimation = Animation::init("./vampire/dancing_vampire.dae", model.bone_info_map);
	auto danceAnimation = Animation::init("./Dancing Twerk.dae", model.bone_info_map);
	auto animator = Animator::init(&danceAnimation);
	assert(animator.bone_matrices.size() <= MAX_BONE_MATRICES);

	Model obj = Model::init("./crate/Old_Crate.fbx", false);
	obj.hitbox = model.hitbox;
	obj.hitbox.min.z += 0.4f;
	obj.hitbox.max.z += 0.4f;

	std::vector<Model> objs(8);
	obj.pos.x += 10;
	objs[0] = obj;
	obj.pos.x += 1;
	obj.pos.z += 1;
	objs[1] = obj;
	obj.pos.x += 1;
	obj.pos.x += 1;
	obj.pos.y += 1;
	obj.pos.z += 1;
	objs[2] = obj;
	obj.pos.x += 1;
	obj.pos.z += 2;
	objs[3] = obj;
	obj.pos.x += 1;
	objs[4] = obj;
	obj.pos.y += 1;
	obj.pos.x += 1;
	objs[5] = obj;
	obj.pos.x += 1;
	obj.pos.y += 1;
	obj.pos.z += 1;
	objs[6] = obj;
	obj.pos.x += 1;
	obj.pos.z += 2;
	objs[7] = obj;

	glBindVertexArray(vao);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

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

			animator.updateAnimation(state.dt/1000000.0f);
		}
		{ // render
			glClearColor(0.0f, 0.0f, 0.0f, 1.00f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glUseProgram(model_anim_shader);

			const auto& transforms = animator.bone_matrices;
			glProgramUniformMatrix4fv(model_anim_shader, model_anim_shader_bone_matrices, transforms.size(), false, glm::value_ptr(transforms[0]));

			state.updateModel(model.pos, vec3(1.0f), vec2(state.view.front.x, state.view.front.z));
			state.updateUB(model.pos);
			state.uploadUB(ubo);
			model.draw(vao, model_anim_shader);

			glUseProgram(model_shader);
			for (auto o : objs) {
				state.updateModel(o.pos, vec3(1.0f), vec2(0.0f));
				state.uploadModel(ubo);
				o.draw(vao, model_shader);
			}
		}
		glfwSwapBuffers(window);
	}

	glDeleteProgram(model_shader);
	glDeleteProgram(model_anim_shader);

	// TODO: free stuff
	// glDeleteVertexArrays(, );
	// glDeleteBuffers(, );

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
