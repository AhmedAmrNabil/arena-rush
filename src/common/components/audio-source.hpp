#pragma once

#include <AL/al.h>

#include <string>

#include "../ecs/component.hpp"

namespace our {
    class AudioSourceComponent : public Component {
    public:
        // Serialized data
        std::string soundName;       // e.g., "gunshot"
        float gain = 1.0f;           // Volume multiplier
        float pitch = 1.0f;          // Speed multiplier
        bool loop = false;           // Whether to keep looping
        bool spatial = false;        // 3D sptatialization
        bool playOnStart = false;    // whether the sound should start playing as soon as the component is created
        float refDistance = 5.0f;    // full volume within this distance
        float maxDistance = 100.0f;  // Attentuation limit

        // Runtime data
        ALuint alSource = 0;  // The AL source assigned to this component (0 = unassigned)

        static std::string getID() {
            return "AudioSource";
        }

        void deserialize(const nlohmann::json& data) override;
    };

}  // namespace our
