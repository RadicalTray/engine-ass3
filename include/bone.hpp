#pragma once

/* Container for bone data */

#include <vector>

#include <assimp/scene.h>

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <utils.hpp>

float getFactor(float last_timestamp, float next_timestamp, float dt);

struct KeyPosition {
	glm::vec3 pos;
	float timestamp;
};

struct KeyRotation {
	glm::quat rot;
	float timestamp;
};

struct KeyScale {
	glm::vec3 scale;
	float timestamp;
};

struct Bone {
	std::vector<KeyPosition> positions;
	std::vector<KeyRotation> rotations;
	std::vector<KeyScale> scales;
	glm::mat4 local_transform;
	std::string name;
	int id;

	static Bone init(const std::string& name, int id, const aiNodeAnim* channel) {
		std::vector<KeyPosition> positions;
		positions.reserve(channel->mNumRotationKeys);
		for (uint i = 0; i < channel->mNumRotationKeys; i++) {
			positions.push_back({
				.pos = glmFromAssimpVec3(channel->mPositionKeys[i].mValue),
				.timestamp = (float)channel->mPositionKeys[i].mTime,
			});
		}

		std::vector<KeyRotation> rotations;
		rotations.reserve(channel->mNumRotationKeys);
		for (uint i = 0; i < channel->mNumRotationKeys; i++) {
			rotations.push_back({
				.rot = glmFromAssimpQuat(channel->mRotationKeys[i].mValue),
				.timestamp = (float)channel->mRotationKeys[i].mTime,
			});
		}

		std::vector<KeyScale> scales;
		scales.reserve(channel->mNumScalingKeys);
		for (uint i = 0; i < channel->mNumScalingKeys; i++) {
			scales.push_back({
				.scale = glmFromAssimpVec3(channel->mScalingKeys[i].mValue),
				.timestamp = (float)channel->mScalingKeys[i].mTime,
			});
		}

		return {
			.positions = positions,
			.rotations = rotations,
			.scales = scales,
			.local_transform = mat4(1.0f),
			.name = name,
			.id = id,
		};
	}
	
	void update(float dt) {
		this->local_transform = interpolatePos(dt) * interpolateRot(dt) * interpolateScale(dt);
	}

	glm::mat4 interpolatePos(float dt) const {
		if (this->positions.size() == 1) {
			return glm::translate(glm::mat4(1.0f), this->positions[0].pos);
		}

		usize idx = this->getPosIdx(dt);
		float factor = getFactor(this->positions[idx].timestamp, this->positions[idx + 1].timestamp, dt);
		glm::vec3 finalPosition = glm::mix(this->positions[idx].pos, this->positions[idx + 1].pos, factor);
		return glm::translate(glm::mat4(1.0f), finalPosition);
	}

	glm::mat4 interpolateRot(float dt) const {
		if (this->rotations.size() == 1) {
			return glm::toMat4(glm::normalize(this->rotations[0].rot));
		}

		usize idx = this->getRotIdx(dt);
		float factor = getFactor(this->rotations[idx].timestamp, this->rotations[idx + 1].timestamp, dt);
		glm::quat finalRotation = glm::normalize(glm::slerp(this->rotations[idx].rot, this->rotations[idx + 1].rot, factor));
		return glm::toMat4(finalRotation);

	}

	glm::mat4 interpolateScale(float dt) const {
		if (this->scales.size() == 1) {
			return glm::scale(glm::mat4(1.0f), this->scales[0].scale);
		}

		usize idx = this->getScaleIdx(dt);
		float factor = getFactor(this->scales[idx].timestamp, this->scales[idx + 1].timestamp, dt);
		glm::vec3 finalScale = glm::mix(this->scales[idx].scale, this->scales[idx + 1].scale, factor);
		return glm::scale(glm::mat4(1.0f), finalScale);
	}

	// Why do these all skip i=0 ???
	usize getPosIdx(float dt) const {
		for (usize i = 1; i < this->positions.size(); i++) {
			if (dt < this->positions[i].timestamp) {
				return i;
			}
		}
		assert(0);
	}

	int getRotIdx(float dt) const {
		for (usize i = 1; i < this->rotations.size(); i++) {
			if (dt < this->rotations[i].timestamp) {
				return i;
			}
		}
		assert(0);
	}

	int getScaleIdx(float dt) const {
		for (usize i = 1; i < this->scales.size(); i++) {
			if (dt < this->scales[i].timestamp) {
				return i;
			}
		}
		assert(0);
	}
};

float getFactor(float last_timestamp, float next_timestamp, float dt) {
	return (dt - last_timestamp) / (next_timestamp - last_timestamp);
}
