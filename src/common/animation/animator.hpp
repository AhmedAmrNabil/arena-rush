#pragma once

#include "animation.hpp"

namespace our {

#define MAX_BONES 110

    class Animator {
        const Animation* currentAnimation;
        float currentTime;  // in ticks
        std::vector<glm::mat4> finalBoneMatrices;
        float deltaTime;

        void computeBoneTransform(const SkeletonNode& node, glm::mat4 parentTransform) {
            const Skeleton& skeleton = currentAnimation->skeleton;
            glm::mat4 nodeTransform = node.localTransform;  // fallback: bind pose

            if (skeleton.hasBone(node.name)) {
                BoneID id = skeleton.getBoneID(node.name);
                auto it = currentAnimation->bones.find(id);
                if (it != currentAnimation->bones.end()) {
                    nodeTransform = it->second.interpolate(currentTime);
                }
            }

            glm::mat4 globalTransform = parentTransform * nodeTransform;

            if (skeleton.hasBone(node.name)) {
                BoneID id = skeleton.getBoneID(node.name);
                glm::mat4 offset = skeleton.getOffsetMatrix(node.name);
                if (id >= 0 && id < static_cast<BoneID>(finalBoneMatrices.size())) {
                    finalBoneMatrices[id] = skeleton.getGlobalInverseTransform() * globalTransform * offset;
                }
            }

            for (const SkeletonNode& child : node.children) computeBoneTransform(child, globalTransform);
        }

    public:
        Animator() : finalBoneMatrices(MAX_BONES, glm::mat4(1.0f)) {}

        void play(const Animation* anim) {
            currentAnimation = anim;
            currentTime = 0.0f;
        }

        void update(float deltaTime, bool loop = true) {
            if (!currentAnimation) return;
            currentTime += currentAnimation->ticksPerSecond * deltaTime;
            if (loop) {
                currentTime = fmod(currentTime, currentAnimation->duration);
            } else {
                currentTime = std::min(currentTime, currentAnimation->duration);
            }
            computeBoneTransform(currentAnimation->skeleton.getRoot(), glm::mat4(1.0f));
        }

        const std::vector<glm::mat4>& getFinalBoneMatrices() const {
            return finalBoneMatrices;
        }

        bool isFinished() const {
            return currentAnimation && currentTime >= currentAnimation->duration;
        }
    };
}  // namespace our
