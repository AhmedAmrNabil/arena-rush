#include "audio-utils.hpp"

#include <AL/al.h>

#define DR_WAV_IMPLEMENTATION
#include <dr_wav/dr_wav.h>

#include <iostream>
#include <stdexcept>
#include <string>

our::AudioBuffer* our::audio_utils::loadWAV(const std::string& filename) {
    unsigned int channels = 0;
    unsigned int sampleRate = 0;
    drwav_uint64 totalFrames = 0;

    // Load WAV file and decode it into 16-bit PCM format
    drwav_int16* samples =
        drwav_open_file_and_read_pcm_frames_s16(filename.c_str(), &channels, &sampleRate, &totalFrames, nullptr);
    if (!samples) {
        std::cerr << "Failed to load WAV file: " << filename << std::endl;
        return nullptr;
    }

    // Determine the OpenAL format based on the number of channels
    ALenum format;
    if (channels == 1) {
        format = AL_FORMAT_MONO16;
    } else if (channels == 2) {
        format = AL_FORMAT_STEREO16;
    } else {
        std::cerr << "Unsupported number of channels in WAV file: " << filename << std::endl;
        drwav_free(samples, nullptr);
        return nullptr;
    }

    // Calculate total byte size of the audio data (totalFrames * channels * bytes per sample)
    ALsizei size = static_cast<ALsizei>(totalFrames * channels * sizeof(drwav_int16));

    // Create the buffer and upload audio to it
    AudioBuffer* audioBuffer = new AudioBuffer();
    audioBuffer->setData(format, samples, size, static_cast<ALsizei>(sampleRate));

    // now free the decoded audio data since it's already copied to OpenAL buffer
    drwav_free(samples, nullptr);

    // Check for OpenAL errors
    ALenum error = alGetError();
    if (error != AL_NO_ERROR) {
        std::cerr << "OpenAL error while loading WAV file: " << filename << ", error code: " << error << std::endl;
        delete audioBuffer;
        return nullptr;
    }

    return audioBuffer;
}
