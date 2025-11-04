#pragma once

#include <vector>
#include <map>
#include <functional>

#include <glm/glm.hpp>

#include <assimp/scene.h>

#include <bone.hpp>
#include <utils.hpp>
#include <model.hpp>

struct AssimpNode {
	glm::mat4 transform;
	std::string name;
	std::vector<AssimpNode> children;

	static AssimpNode init(const aiNode* src) {
		std::vector<AssimpNode> children;
		children.reserve(src->mNumChildren);
		for (uint i = 0; i < src->mNumChildren; i++) {
			children.push_back(AssimpNode::init(src->mChildren[i]));
		}

		return {
			.transform = glmFromAssimpMat4(src->mTransformation),
			.name = src->mName.data,
			.children = children,
		};
	}
};

struct Animation {
	std::vector<Bone> bones;
	std::map<std::string, BoneInfo>& bone_info_map;
	float duration;
	float ticks_per_sec;
	AssimpNode root_node;

	static Animation init(const std::string& filepath, std::map<std::string, BoneInfo>& bone_info_map) {
		Assimp::Importer imp;
		const aiScene *scene = imp.ReadFile(filepath, aiProcess_Triangulate);
		if (!(scene && scene->mRootNode)) {
			std::cerr << "assimp(error): " << imp.GetErrorString() << std::endl;
			return {
				.bone_info_map = bone_info_map,
			}; // TODO: handle error
		}

		auto anim = scene->mAnimations[0];

		// populate bones and also add missing bones
		std::vector<Bone> bones;
		bones.reserve(anim->mNumChannels);
		for (uint i = 0; i < anim->mNumChannels; i++) {
			auto channel = anim->mChannels[i];
			std::string bone_name = channel->mNodeName.data;

			// don't need to set offset cuz not used ig
			if (!bone_info_map.contains(bone_name)) {
				int id = bone_info_map.size();
				bone_info_map[bone_name] = { .id = id };
			}

			bones.push_back(Bone::init(bone_name, bone_info_map[bone_name].id, channel));
		}

		return {
			.bones = bones,
			.bone_info_map = bone_info_map,
			.duration = (float)anim->mDuration,
			.ticks_per_sec = (float)anim->mTicksPerSecond,
			.root_node = AssimpNode::init(scene->mRootNode),
		};
	}

	Bone* findBone(const std::string& name) {
		// TODO: Do we need this wtf
		auto iter = std::find_if(this->bones.begin(), this->bones.end(), [&](const Bone& bone) { return bone.name == name; });
		if (iter == this->bones.end()) {
			return nullptr;
		}
		return &(*iter);
	}
};
