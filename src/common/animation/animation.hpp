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
        std::unordered_map<BoneID, BoneAnimation> bones;
        Skeleton& skeleton;
        float duration;  // in ticks
        float ticksPerSecond;
        Animation(const aiAnimation* anim, Skeleton& skeleton) : skeleton(skeleton) {
            duration = static_cast<float>(anim->mDuration);
            ticksPerSecond = static_cast<float>(anim->mTicksPerSecond);

            for (unsigned int i = 0; i < anim->mNumChannels; i++) {
                aiNodeAnim* channel = anim->mChannels[i];
                std::string boneName = channel->mNodeName.C_Str();

                // find or create because some bones are not referenced in the meshes but are still animated
                // they don't contribute to mesh weights but they still affect the bones after them in the hierarchy
                BoneID id = skeleton.findOrCreateBone(boneName, glm::mat4(1.0f));
                bones.emplace(id, BoneAnimation(boneName, id, channel));
            }
        }
        BoneAnimation& findBone(const std::string& name) {
            auto it = bones.find(skeleton.getBoneID(name));
            if (it != bones.end()) {
                return it->second;
            }
            throw std::runtime_error("BoneAnimation not found: " + name);
        }
    };
}  // namespace our
