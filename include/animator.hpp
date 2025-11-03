#pragma once

#include <map>
#include <vector>
#include <cmath>

#include <glm/glm.hpp>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>

#include <animation.hpp>
#include <bone.hpp>

// TODO: multiple animations
struct Animator {
	std::vector<glm::mat4> bone_matrices;
	Animation* curr_anim;
	float curr_time;

	static Animator init(Animation* animation) {
		return {
			.bone_matrices = std::vector<glm::mat4>(MAX_BONE_MATRICES, glm::mat4(1.0f)),
			.curr_anim = animation,
			.curr_time = 0,
		};
	}

	void updateAnimation(float dt) {
		this->curr_time = std::fmod(this->curr_time + this->curr_anim->ticks_per_sec * dt, this->curr_anim->duration);
		this->calculateBoneTransform(&this->curr_anim->root_node, glm::mat4(1.0f));
	}

	void calculateBoneTransform(const AssimpNode* node, const glm::mat4& parent_transform) {
		std::string node_name = node->name;
		glm::mat4 node_transform = node->transform;

		auto bone = this->curr_anim->findBone(node_name);
		if (bone) {
			bone->update(this->curr_time);
			node_transform = bone->local_transform;
		}

		glm::mat4 global_transform = parent_transform * node_transform;

		auto& bone_info_map = this->curr_anim->bone_info_map;
		if (bone_info_map.contains(node_name)) {
			bone_matrices[bone_info_map[node_name].id] = global_transform * bone_info_map[node_name].offset;
		}

		for (usize i = 0; i < node->children.size(); i++) {
			this->calculateBoneTransform(&node->children[i], global_transform);
		}
	}
};
