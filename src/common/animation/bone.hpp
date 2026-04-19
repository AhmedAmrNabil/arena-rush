#pragma once

#include <assimp/anim.h>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <string>
#include <vector>

namespace our {
    using BoneID = int;
    struct BonePose {
        BoneID id;
        glm::mat4 offsetMatrix;  // the inverse of the bone's bind pose transform
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

    class BoneAnimation {
        std::vector<KeyPosition> positions;
        std::vector<KeyRotation> rotations;
        std::vector<KeyScale> scales;
        std::string name;

        template <typename TKey>
        static size_t findIndex(const std::vector<TKey>& keys, float animationTime) {
            for (size_t i = 0; i < keys.size() - 1; ++i)
                if (animationTime < keys[i + 1].time) return i;
            return keys.size() - 2;
        }

        template <typename TKey, typename TGet, typename TInterp>
        static auto interpolateKeys(const std::vector<TKey>& keys, float animationTime, TInterp interp, TGet getValue) {
            if (keys.size() == 1) return getValue(keys[0]);
            if (animationTime >= keys.back().time) return getValue(keys.back());

            size_t i0 = findIndex(keys, animationTime);
            size_t i1 = i0 + 1;
            float factor = (animationTime - keys[i0].time) / (keys[i1].time - keys[i0].time);
            return interp(getValue(keys[i0]), getValue(keys[i1]), factor);
        }

        template <typename TKey, typename TAssimp, typename TExtract>
        static std::vector<TKey> loadKeys(unsigned int count, TAssimp* keys, TExtract extract) {
            std::vector<TKey> result;
            result.reserve(count);
            for (unsigned int i = 0; i < count; ++i) {
                result.emplace_back(TKey{extract(keys[i]), static_cast<float>(keys[i].mTime)});
            }
            return result;
        }

    public:
        BoneAnimation() = default;
        BoneAnimation(const std::string& name, BoneID id, const aiNodeAnim* channel) : name(name) {
            // Load position keyframes
            positions = loadKeys<KeyPosition>(
                channel->mNumPositionKeys, channel->mPositionKeys,
                [](const aiVectorKey& k) { return glm::vec3(k.mValue.x, k.mValue.y, k.mValue.z); });

            rotations = loadKeys<KeyRotation>(
                channel->mNumRotationKeys, channel->mRotationKeys,
                [](const aiQuatKey& k) { return glm::quat(k.mValue.w, k.mValue.x, k.mValue.y, k.mValue.z); });

            scales = loadKeys<KeyScale>(channel->mNumScalingKeys, channel->mScalingKeys, [](const aiVectorKey& k) {
                return glm::vec3(k.mValue.x, k.mValue.y, k.mValue.z);
            });
        }

        glm::mat4 interpolate(float animationTime) const {
            glm::vec3 interpolatedPosition = interpolateKeys(
                positions, animationTime, [](auto a, auto b, float f) { return glm::mix(a, b, f); },
                [](const KeyPosition& k) { return k.position; });
            glm::quat interpolatedRotation = interpolateKeys(
                rotations, animationTime, [](auto a, auto b, float f) { return glm::slerp(a, b, f); },
                [](const KeyRotation& k) { return k.rotation; });
            glm::vec3 interpolatedScale = interpolateKeys(
                scales, animationTime, [](auto a, auto b, float f) { return glm::mix(a, b, f); },
                [](const KeyScale& k) { return k.scale; });

            return glm::translate(glm::mat4(1.0f), interpolatedPosition) * glm::toMat4(interpolatedRotation) *
                   glm::scale(glm::mat4(1.0f), interpolatedScale);
        }
    };
}  // namespace our
