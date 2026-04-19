#pragma once

#include <assimp/scene.h>

#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "bone.hpp"

namespace our {

    struct SkeletonNode {
        std::string name;
        glm::mat4 localTransform;  // bind pose transform
        std::vector<SkeletonNode> children;
    };

    class Skeleton {
        std::unordered_map<std::string, BoneInfo> boneInfoMap;  // maps bone names to their corresponding BoneInfo
        SkeletonNode root;                                      // the root of the skeleton hierarchy
        glm::mat4 globalInverseTransform = glm::mat4(1.0f);     // inverse of the scene root transform
        void buildHierarchy(SkeletonNode& skeletonNode, aiNode* node);

    public:
        BoneID findOrCreateBone(const std::string& boneName, aiMatrix4x4& offsetMatrix);
        void buildHierarchy(aiNode* node);
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
        SkeletonNode& getRoot() {
            return root;
        }

        // used for FBX files where the root node's transform is not identity
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
