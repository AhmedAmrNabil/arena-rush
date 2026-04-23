#pragma once

#include <cmath>  // for std::fmod
#include <glm/glm.hpp>
#include <queue>
#include <unordered_map>
#include <vector>

#include "animation.hpp"

namespace our {

#define MAX_BONES 110

    class Animator {
        const Animation* currentAnimation = nullptr;
        float currentTime;  // in ticks
        float speed;        // animation speed
        // there are 2 types of animations that assimp supports
        // 1. skeletal animations that affect the bones and are used for skinning meshes
        // 2. node animations that affect any node in the hierarchy but are not used for skinning
        //    (eg. a whole mesh rotating as a child of a bone, or a light source attached to a bone)
        std::vector<glm::mat4> finalBoneMatrices;                   // for bone animations
        std::unordered_map<std::string, glm::mat4> nodeTransforms;  // for node animations
        bool loop;

        void computeBoneTransform(const std::vector<SkeletonNode>& nodes) {
            const Skeleton& skeleton = currentAnimation->skeleton;
            std::vector<glm::mat4> globalTransforms(nodes.size());
            nodeTransforms.clear();
            glm::mat4 globalInverse = skeleton.getGlobalInverseTransform();

            for (int i = 0; i < (int)nodes.size(); i++) {
                const auto& node = nodes[i];

                // animated local transform, fallback to bind pose
                glm::mat4 localTransform = node.localTransform;
                auto it = currentAnimation->channels.find(node.name);
                if (it != currentAnimation->channels.end()) {
                    localTransform = it->second.interpolate(currentTime);
                }

                // accumulate from parent
                if (node.parentIndex < 0)
                    globalTransforms[i] = localTransform;
                else
                    globalTransforms[i] = globalTransforms[node.parentIndex] * localTransform;

                // Cache for meshes
                nodeTransforms[node.name] = globalTransforms[i];

                // Cache for skeletal bones
                BoneID id = skeleton.getBoneID(node.name);
                if (id >= 0 && id < (BoneID)finalBoneMatrices.size())
                    finalBoneMatrices[id] = globalInverse * globalTransforms[i] * skeleton.getOffsetMatrix(node.name);
            }
        }

    public:
        Animator() : finalBoneMatrices(MAX_BONES, glm::mat4(1.0f)) {}

        // speed should be affected by the scale and movement speed of the real entity
        void play(const Animation* anim, bool loop = true, float speed = 1.0f) {
            currentAnimation = anim;
            currentTime = 0.0f;
            this->loop = loop;
            this->speed = speed;
        }

        void update(float deltaTime) {
            if (!currentAnimation) return;
            currentTime += currentAnimation->ticksPerSecond * speed * deltaTime;
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

        const glm::mat4* getNodeTransform(const std::string& name) const {
            auto it = nodeTransforms.find(name);
            if (it != nodeTransforms.end()) return &it->second;
            return nullptr;
        }

        bool isFinished() const {
            return currentAnimation && currentTime >= currentAnimation->duration && !loop;
        }
    };
}  // namespace our
