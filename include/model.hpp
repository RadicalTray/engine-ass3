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
	std::map<std::string, BoneInfo> bone_info_map;
	uint vao;
	uint shader;

	// vec3 front; // follow cam

	static Model init(const std::string& path, uint vao, uint shader) {
		Model model = {};
		model.vao = vao;
		model.shader = shader;

		uint flags = 0;
		flags |= aiProcess_Triangulate;
		// flags |= aiProcess_JoinIdenticalVertices;
		// flags |= aiProcess_CalcTangentSpace;

		std::cerr << "assimp(info): loading " << path << std::endl;
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

	void draw() const {
		for (auto m : this->meshes) {
			m.draw(this->vao, this->shader);
		}
	}
};
