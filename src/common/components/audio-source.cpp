#include "audio-source.hpp"

namespace our {
    void AudioSourceComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;

        if (data.contains("soundName")) soundName = data["soundName"].get<std::string>();
        if (data.contains("gain")) gain = data["gain"].get<float>();
        if (data.contains("pitch")) pitch = data["pitch"].get<float>();
        if (data.contains("loop")) loop = data["loop"].get<bool>();
        if (data.contains("spatial")) spatial = data["spatial"].get<bool>();
        if (data.contains("playOnStart")) playOnStart = data["playOnStart"].get<bool>();
        if (data.contains("refDistance")) refDistance = data["refDistance"].get<float>();
        if (data.contains("maxDistance")) maxDistance = data["maxDistance"].get<float>();
    }

}  // namespace our
