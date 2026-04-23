#pragma once

#include <AL/al.h>
#include <AL/alc.h>

#include <glm/glm.hpp>
#include <iostream>
#include <string>
#include <vector>

#include "../components/audio-source.hpp"
#include "../ecs/world.hpp"

namespace our {
    class AudioBuffer;

    class AudioSystem {
    public:
        void initialize();
        void update(World* world);
        void destroy();

        // On-demand function to play a sound at a 3D world position (fire-and-forget)
        // Play a sound at a 3D world position (fire-and-forget)
        // - buffer: the AudioBuffer containing the sound data to play
        // - position: the 3D world position to play the sound at
        // - gain: volume multiplier
        // - pitch: speed multiplier
        // - loop: whether to keep looping the sound
        ALuint playSound(AudioBuffer* buffer, const glm::vec3& position, float gain, float pitch, bool loop,
                         float refDistance, float maxDistance);

        // On-demand function to play a non-spatial sound (fire-and-forget)
        // Best used for UI, menu, or music sounds
        ALuint playSound2D(AudioBuffer* buffer, float gain, float pitch, bool loop);

        // On-demand function to check if a sound source is currently playing
        bool isPlaying(ALuint source);

        // On-demand function to set the master (listener) volume for all sounds
        void setMasterVolume(float volume);

        // On-demand function to stop a specific sound source
        void stopSound(ALuint source);

        // On-demand function to stop all sounds
        void stopAll();

    private:
        ALCdevice* device = nullptr;
        ALCcontext* context = nullptr;
        static constexpr int SOURCE_POOL_SIZE = 32;  // Maximum number of simultaneous sounds
        std::vector<ALuint> sourcePool;              // Pool of available AL sources
        size_t nextSourceIndex = 0;

        ALuint findAvailableSource();

        void startSource(AudioSourceComponent* audioSource, Entity* entity);

        // Helper function to check for OpenAL errors
        void checkALError(const std::string& context) {
            ALenum error = alGetError();
            if (error != AL_NO_ERROR) {
                std::cerr << "OpenAL error in " << context << ": " << error << std::endl;
            }
        }
    };

}  // namespace our
