#pragma once

#include <string>

#include "audio-buffer.hpp"
namespace our::audio_utils {
    AudioBuffer* loadWAV(const std::string& filename);

    // TODO: Add more functions for loading other audio formats (most likely OGG and maybe MP3)

}  // namespace our::audio_utils
