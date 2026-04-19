#pragma once

#include "animation.hpp"

namespace our {

#define MAX_BONES 110

    class Animator {
        const Animation* currentAnimation;
        float currentTime;  // in ticks
        std::vector<glm::mat4> finalBoneMatrices;
        bool loop;

        void computeBoneTransform(const std::vector<SkeletonNode>& nodes) {
            const Skeleton& skeleton = currentAnimation->skeleton;
            std::vector<glm::mat4> globalTransforms(nodes.size());

            for (int i = 0; i < (int)nodes.size(); i++) {
                const auto& node = nodes[i];

                // animated local transform, fallback to bind pose
                glm::mat4 localTransform = node.localTransform;
                BoneID id = skeleton.getBoneID(node.name);
                if (id >= 0) {
                    auto it = currentAnimation->bones.find(id);
                    if (it != currentAnimation->bones.end()) localTransform = it->second.interpolate(currentTime);
                }

                // accumulate from parent
                if (node.parentIndex < 0)
                    globalTransforms[i] = localTransform;
                else
                    globalTransforms[i] = globalTransforms[node.parentIndex] * localTransform;

                if (id >= 0 && id < (BoneID)finalBoneMatrices.size())
                    finalBoneMatrices[id] = skeleton.getGlobalInverseTransform() * globalTransforms[i] *
                                            skeleton.getOffsetMatrix(node.name);
            }
        }

    public:
        Animator() : finalBoneMatrices(MAX_BONES, glm::mat4(1.0f)) {}

        void play(const Animation* anim, bool loop = true) {
            currentAnimation = anim;
            currentTime = 0.0f;
            this->loop = loop;
        }

        void update(float deltaTime) {
            if (!currentAnimation) return;
            currentTime += currentAnimation->ticksPerSecond * deltaTime;
            if (loop) {
                currentTime = std::fmod(currentTime, currentAnimation->duration);
            } else {
                currentTime = std::min(currentTime, currentAnimation->duration);
            }
            computeBoneTransform(currentAnimation->skeleton.getNodes());
        }

        const std::vector<glm::mat4>& getFinalBoneMatrices() const {
            return finalBoneMatrices;
        }

        bool isFinished() const {
            return currentAnimation && currentTime >= currentAnimation->duration;
        }
    };
}  // namespace our
