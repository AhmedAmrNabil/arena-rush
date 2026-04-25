#pragma once

#include <assimp/scene.h>

#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "bone.hpp"
#include "model/ai-glm-utils.hpp"

namespace our {

    struct SkeletonNode {
        std::string name;
        glm::mat4 localTransform;  // bind pose transform of this node relative to its parent
        int parentIndex = -1;      // index of the parent node in the Skeleton's nodes vector, -1 if this is the root
    };

    class Skeleton {
        std::unordered_map<std::string, BonePose> boneInfoMap;  // maps bone names to their corresponding BonePose
        glm::mat4 globalInverseTransform = glm::mat4(1.0f);     // inverse of the scene root transform
        std::vector<SkeletonNode> nodes;                        // flat list of all skeleton nodes (for easy traversal)

    public:
        Skeleton() = default;
        BoneID findOrCreateBone(const std::string& boneName, aiMatrix4x4& offsetMatrix) {
            return findOrCreateBone(boneName, aiToGlm(offsetMatrix));
        }

        BoneID findOrCreateBone(const std::string& boneName, const glm::mat4& offsetMatrix) {
            auto it = boneInfoMap.find(boneName);
            if (it != boneInfoMap.end()) {
                return it->second.id;
            } else {
                BoneID newID = static_cast<BoneID>(boneInfoMap.size());
                boneInfoMap[boneName] = {newID, offsetMatrix};
                return newID;
            }
        }

        bool hasBone(const std::string& boneName) const {
            return boneInfoMap.find(boneName) != boneInfoMap.end();
        }

        BoneID getBoneID(const std::string& boneName) const {
            auto it = boneInfoMap.find(boneName);
            if (it != boneInfoMap.end()) {
                return it->second.id;
            }
            return -1;  // or throw an exception but will handle silently for now
        }

        std::vector<SkeletonNode>& getNodes() {
            return nodes;
        }

        // used for FBX files where the root node's transform is not identity
        // currently its unused since our test models don't have such a case, but it's here for completeness
        void setGlobalInverseTransform(const glm::mat4& inverseTransform) {
            globalInverseTransform = inverseTransform;
        }

        const glm::mat4& getGlobalInverseTransform() const {
            return globalInverseTransform;
        }

        glm::mat4 getOffsetMatrix(const std::string& boneName) const {
            auto it = boneInfoMap.find(boneName);
            if (it != boneInfoMap.end()) {
                return it->second.offsetMatrix;
            }
            return glm::mat4(1.0f);  // identity matrix as fallback
        }
    };
}  // namespace our
