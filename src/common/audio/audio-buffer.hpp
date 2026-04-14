#pragma once

#include <AL/al.h>
#include <dr_wav/dr_wav.h>

#include <stdexcept>
#include <string>

namespace our {
    class AudioBuffer {
        ALuint bufferID = 0;

    public:
        AudioBuffer() {
            alGenBuffers(1, &bufferID);
        }
        ~AudioBuffer() {
            if (bufferID != 0) alDeleteBuffers(1, &bufferID);
        };

        void setData(ALenum format, const void* data, ALsizei size, ALsizei freq) {
            alBufferData(bufferID, format, data, size, freq);
        }

        ALuint getBufferID() const {
            return bufferID;
        }

        // Disable copying
        AudioBuffer(const AudioBuffer&) = delete;
        AudioBuffer& operator=(const AudioBuffer&) = delete;
    };
}  // namespace our
