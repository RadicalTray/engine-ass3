#pragma once
#include <vector>
#include <string>
#include <map>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <utils.hpp>
#include "./mesh.hpp"

struct Box {
	vec3 min;
	vec3 max;

	Box translate(vec3 x) const {
		Box box = *this;
		box.min += x;
		box.max += x;
		return box;
	}
};

struct Model {
	std::vector<TextureInfo> textures_loaded;
	std::vector<Mesh> meshes;
	vec3 velocity;
	vec3 pos;
	Box hitbox;
	std::map<std::string, BoneInfo> bone_info_map;

	// vec3 front; // follow cam

	static Model init(const std::string& path, bool flip_uv) {
		Model model = {};

		uint flags = 0;
		flags |= aiProcess_Triangulate;
		// flags |= aiProcess_JoinIdenticalVertices;
		// flags |= aiProcess_CalcTangentSpace;
		if (flip_uv) flags |= aiProcess_FlipUVs;

		Assimp::Importer imp;
		const aiScene *scene = imp.ReadFile(path, flags);
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			std::cerr << "assimp(error): " << imp.GetErrorString() << std::endl;
			return {}; // TODO: handle error
		}
		auto directory = path.substr(0, path.find_last_of('/')); // doesn't work if a basename/dirname has '/'
		model.processNode(scene->mRootNode, scene, directory);

		return model;
	}

	void processNode(aiNode* node, const aiScene* scene, const std::string& directory) {
		for (uint i = 0; i < node->mNumMeshes; i++) {
			aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
			this->meshes.push_back(Mesh::init(mesh, scene, this->textures_loaded, bone_info_map, directory));
		}
		for (uint i = 0; i < node->mNumChildren; i++) {
			processNode(node->mChildren[i], scene, directory);
		}
	}


	void draw(uint vao, uint shader) const {
		for (auto m : this->meshes) {
			m.draw(vao, shader);
		}
	}

	bool detectObj(vec3& new_pos, Model obj) {
		bool collision = true;

		Box new_box = this->hitbox.translate(new_pos);
		Box obj_box = obj.hitbox.translate(obj.pos);

		// Taken from raylib
		if ((new_box.max.x >= obj_box.min.x) && (new_box.min.x <= obj_box.max.x)) {
			if ((new_box.max.y < obj_box.min.y) || (new_box.min.y > obj_box.max.y)) collision = false;
			if ((new_box.max.z < obj_box.min.z) || (new_box.min.z > obj_box.max.z)) collision = false;
		} else {
			collision = false;
		}

		vec3 old_pos = this->pos;
		Box old_box = this->hitbox.translate(old_pos);
		vec3 diff = new_pos - old_pos;
		if (collision) {
			if (diff.y < 0.0f) {
				// if was on top
				if (old_box.min.y >= obj_box.max.y) {
					new_pos.y = obj_box.max.y;
					this->velocity.y = 0;
				}
			}
		}

		return collision;
	}
};
