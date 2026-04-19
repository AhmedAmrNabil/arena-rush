#pragma once

#include <assimp/scene.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <stdexcept>
#include <string>
#include <vector>

#include "bone.hpp"
#include "skeleton.hpp"

namespace our {

    class Animation {
    public:
        std::string name;
        std::unordered_map<BoneID, Bone> bones;
        Skeleton& skeleton;
        float duration;  // in ticks
        float ticksPerSecond;
        Animation(const aiAnimation* anim, Skeleton& skeleton) : skeleton(skeleton) {
            duration = static_cast<float>(anim->mDuration);
            ticksPerSecond = static_cast<float>(anim->mTicksPerSecond);

            for (unsigned int i = 0; i < anim->mNumChannels; i++) {
                aiNodeAnim* channel = anim->mChannels[i];
                std::string boneName = channel->mNodeName.C_Str();

                // skip nodes that aren't bones (IK targets, camera nodes, etc.)
                if (!skeleton.hasBone(boneName)) continue;

                BoneID id = skeleton.getBoneID(boneName);
                bones.emplace(id, Bone(boneName, id, channel));
            }
        }
        Bone& findBone(const std::string& name) {
            auto it = bones.find(skeleton.getBoneID(name));
            if (it != bones.end()) {
                return it->second;
            }
            throw std::runtime_error("Bone not found: " + name);
        }
    };
}  // namespace our
