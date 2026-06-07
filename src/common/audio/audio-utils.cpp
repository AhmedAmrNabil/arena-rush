#include "audio-utils.hpp"

#include <AL/al.h>

#define DR_WAV_IMPLEMENTATION
#include <dr_wav/dr_wav.h>

#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c>
#undef STB_VORBIS_HEADER_ONLY

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {
    ALenum channelsToFormat(unsigned int channels, const std::string& filename) {
        if (channels == 1) return AL_FORMAT_MONO16;
        if (channels == 2) return AL_FORMAT_STEREO16;
        std::cerr << "Unsupported number of channels (" << channels << ") in audio file: " << filename << std::endl;
        return 0;
    }
}  // namespace

our::AudioBuffer* our::audio_utils::loadWAV(const std::string& filename) {
    unsigned int channels = 0;
    unsigned int sampleRate = 0;
    drwav_uint64 totalFrames = 0;

    drwav_int16* samples =
        drwav_open_file_and_read_pcm_frames_s16(filename.c_str(), &channels, &sampleRate, &totalFrames, nullptr);
    if (!samples) {
        std::cerr << "Failed to load WAV file: " << filename << std::endl;
        return nullptr;
    }

    ALenum format = channelsToFormat(channels, filename);
    if (format == 0) {
        drwav_free(samples, nullptr);
        return nullptr;
    }

    ALsizei size = static_cast<ALsizei>(totalFrames * channels * sizeof(drwav_int16));

    AudioBuffer* audioBuffer = new AudioBuffer();
    audioBuffer->setData(format, samples, size, static_cast<ALsizei>(sampleRate));
    drwav_free(samples, nullptr);

    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        std::cerr << "OpenAL error while loading WAV file: " << filename << ", error code: " << error << std::endl;
        delete audioBuffer;
        return nullptr;
    }
    return audioBuffer;
}

our::AudioBuffer* our::audio_utils::loadOGG(const std::string& filename) {
    int channels = 0;
    int sampleRate = 0;
    short* samples = nullptr;
    int totalSamples = stb_vorbis_decode_filename(filename.c_str(), &channels, &sampleRate, &samples);
    if (totalSamples < 0 || samples == nullptr) {
        std::cerr << "Failed to load OGG file: " << filename << std::endl;
        return nullptr;
    }

    ALenum format = channelsToFormat(static_cast<unsigned int>(channels), filename);
    if (format == 0) {
        std::free(samples);
        return nullptr;
    }

    ALsizei size = static_cast<ALsizei>(totalSamples * channels * sizeof(short));

    AudioBuffer* audioBuffer = new AudioBuffer();
    audioBuffer->setData(format, samples, size, static_cast<ALsizei>(sampleRate));
    std::free(samples);

    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        std::cerr << "OpenAL error while loading OGG file: " << filename << ", error code: " << error << std::endl;
        delete audioBuffer;
        return nullptr;
    }
    return audioBuffer;
}

our::AudioBuffer* our::audio_utils::loadAudio(const std::string& filename) {
    auto dot = filename.find_last_of('.');
    std::string ext = (dot == std::string::npos) ? "" : filename.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
    if (ext == "ogg") return loadOGG(filename);
    if (ext == "wav") return loadWAV(filename);
    std::cerr << "Unsupported audio extension '." << ext << "' for file: " << filename << std::endl;
    return nullptr;
}
