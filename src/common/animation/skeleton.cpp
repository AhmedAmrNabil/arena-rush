#include "skeleton.hpp"

#include <assimp/scene.h>

#include <iostream>

#include "mesh/vertex.hpp"
#include "model/ai-glm-utils.hpp"

namespace our {

    BoneID Skeleton::findOrCreateBone(const std::string& boneName, aiMatrix4x4& offsetMatrix) {
        auto it = boneInfoMap.find(boneName);
        if (it != boneInfoMap.end()) {
            return it->second.id;
        } else {
            BoneID newID = static_cast<BoneID>(boneInfoMap.size());
            boneInfoMap[boneName] = {newID, aiToGlm(offsetMatrix)};
            return newID;
        }
    }

    void Skeleton::buildHierarchy(SkeletonNode& skeletonNode, aiNode* node) {
        skeletonNode.name = node->mName.C_Str();
        skeletonNode.localTransform = aiToGlm(node->mTransformation);
        for (unsigned int i = 0; i < node->mNumChildren; ++i) {
            SkeletonNode childNode;
            buildHierarchy(childNode, node->mChildren[i]);
            skeletonNode.children.push_back(childNode);
        }
    }

    void Skeleton::buildHierarchy(aiNode* node) {
        buildHierarchy(root, node);
    }
}  // namespace our
