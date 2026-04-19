#pragma once

#include <assimp/scene.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "bone.hpp"
#include "skeleton.hpp"

namespace our {

    class Animation {
    public:
        std::string name;
        std::unordered_map<std::string, BoneAnimation> channels;
        Skeleton& skeleton;
        float duration;  // in ticks
        float ticksPerSecond;
        Animation(const aiAnimation* anim, Skeleton& skeleton) : skeleton(skeleton) {
            duration = static_cast<float>(anim->mDuration);
            float tps = static_cast<float>(anim->mTicksPerSecond);
            if (tps == 0.0f) tps = 25.0f;  // default to 25 if not specified
            ticksPerSecond = tps;
            name = anim->mName.C_Str();

            for (unsigned int i = 0; i < anim->mNumChannels; i++) {
                aiNodeAnim* channel = anim->mChannels[i];
                std::string nodeName = channel->mNodeName.C_Str();
                channels[nodeName] = BoneAnimation(nodeName, skeleton.getBoneID(nodeName), channel);
            }
        }
        BoneAnimation& findBone(const std::string& name) {
            auto it = channels.find(name);
            if (it != channels.end()) {
                return it->second;
            }
            throw std::runtime_error("BoneAnimation not found: " + name);
        }
    };
}  // namespace our
