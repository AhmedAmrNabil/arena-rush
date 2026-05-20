#pragma once

#include <string>

#include "audio-buffer.hpp"
namespace our::audio_utils {
    AudioBuffer* loadWAV(const std::string& filename);
    AudioBuffer* loadOGG(const std::string& filename);
    AudioBuffer* loadAudio(const std::string& filename);

}  // namespace our::audio_utils
