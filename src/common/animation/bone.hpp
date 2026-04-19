#pragma once

#include <assimp/anim.h>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <string>

namespace our {
    using BoneID = int;
    struct BoneInfo {
        BoneID id;
        glm::mat4 offsetMatrix;
    };

    struct KeyPosition {
        glm::vec3 position;
        float time;
    };

    struct KeyRotation {
        glm::quat rotation;
        float time;
    };

    struct KeyScale {
        glm::vec3 scale;
        float time;
    };

    class Bone {
        std::string boneName;
        std::vector<KeyPosition> positions;
        std::vector<KeyRotation> rotations;
        std::vector<KeyScale> scales;
        std::string name;
        BoneID id;

        glm::vec3 interpolatePosition(float animationTime) const {
            if (positions.size() == 1) return positions[0].position;
            if (animationTime >= positions.back().time) return positions.back().position;

            int p0Index = 0;
            for (size_t i = 0; i < positions.size() - 1; ++i) {
                if (animationTime < positions[i + 1].time) {
                    p0Index = i;
                    break;
                }
            }
            int p1Index = p0Index + 1;
            float deltaTime = positions[p1Index].time - positions[p0Index].time;
            float factor = (animationTime - positions[p0Index].time) / deltaTime;
            return glm::mix(positions[p0Index].position, positions[p1Index].position, factor);
        }

        glm::quat interpolateRotation(float animationTime) const {
            if (rotations.size() == 1) return rotations[0].rotation;
            if (animationTime >= rotations.back().time) return rotations.back().rotation;

            int r0Index = 0;
            for (size_t i = 0; i < rotations.size() - 1; ++i) {
                if (animationTime < rotations[i + 1].time) {
                    r0Index = i;
                    break;
                }
            }
            int r1Index = r0Index + 1;
            float deltaTime = rotations[r1Index].time - rotations[r0Index].time;
            float factor = (animationTime - rotations[r0Index].time) / deltaTime;
            return glm::slerp(rotations[r0Index].rotation, rotations[r1Index].rotation, factor);
        }

        glm::vec3 interpolateScale(float animationTime) const {
            if (scales.size() == 1) return scales[0].scale;
            if (animationTime >= scales.back().time) return scales.back().scale;

            int s0Index = 0;
            for (size_t i = 0; i < scales.size() - 1; ++i) {
                if (animationTime < scales[i + 1].time) {
                    s0Index = i;
                    break;
                }
            }
            int s1Index = s0Index + 1;
            float deltaTime = scales[s1Index].time - scales[s0Index].time;
            float factor = (animationTime - scales[s0Index].time) / deltaTime;
            return glm::mix(scales[s0Index].scale, scales[s1Index].scale, factor);
        }

    public:
        Bone(const std::string& name, BoneID id, const aiNodeAnim* channel) : name(name), id(id) {
            boneName = channel->mNodeName.C_Str();

            // Load position keyframes
            for (unsigned int i = 0; i < channel->mNumPositionKeys; ++i) {
                KeyPosition kp;
                kp.position = glm::vec3(channel->mPositionKeys[i].mValue.x, channel->mPositionKeys[i].mValue.y,
                                        channel->mPositionKeys[i].mValue.z);
                kp.time = static_cast<float>(channel->mPositionKeys[i].mTime);
                positions.push_back(kp);
            }

            // Load rotation keyframes
            for (unsigned int i = 0; i < channel->mNumRotationKeys; ++i) {
                KeyRotation kr;
                kr.rotation = glm::quat(channel->mRotationKeys[i].mValue.w, channel->mRotationKeys[i].mValue.x,
                                        channel->mRotationKeys[i].mValue.y, channel->mRotationKeys[i].mValue.z);
                kr.time = static_cast<float>(channel->mRotationKeys[i].mTime);
                rotations.push_back(kr);
            }

            // Load scale keyframes
            for (unsigned int i = 0; i < channel->mNumScalingKeys; ++i) {
                KeyScale ks;
                ks.scale = glm::vec3(channel->mScalingKeys[i].mValue.x, channel->mScalingKeys[i].mValue.y,
                                     channel->mScalingKeys[i].mValue.z);
                ks.time = static_cast<float>(channel->mScalingKeys[i].mTime);
                scales.push_back(ks);
            }
        }
        glm::mat4 interpolate(float animationTime) const {
            glm::vec3 interpolatedPosition = interpolatePosition(animationTime);
            glm::quat interpolatedRotation = interpolateRotation(animationTime);
            glm::vec3 interpolatedScale = interpolateScale(animationTime);

            return glm::translate(glm::mat4(1.0f), interpolatedPosition) * glm::toMat4(interpolatedRotation) *
                   glm::scale(glm::mat4(1.0f), interpolatedScale);
        }
    };
}  // namespace our
