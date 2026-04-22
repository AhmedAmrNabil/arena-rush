#include "audio-system.hpp"

#include <iostream>

#include "../asset-loader.hpp"
#include "../audio/audio-buffer.hpp"
#include "../components/camera.hpp"

namespace our {
    void AudioSystem::initialize() {
        // Open default audio device
        device = alcOpenDevice(nullptr);
        if (!device) {
            std::cerr << "Failed to open OpenAL device" << std::endl;
            return;
        }

        // Print the opened device name
        if (alcIsExtensionPresent(nullptr, "ALC_ENUMERATE_ALL_EXT")) {
            const ALCchar* realDeviceName = alcGetString(device, ALC_ALL_DEVICES_SPECIFIER);
            std::cout << "[Audio] Opened device: " << realDeviceName << std::endl;
        }

        // Create OpenAL context
        context = alcCreateContext(device, nullptr);
        if (!context) {
            std::cerr << "Failed to create OpenAL context" << std::endl;
            alcCloseDevice(device);
            device = nullptr;
            return;
        }

        // Make the context current
        if (!alcMakeContextCurrent(context)) {
            std::cerr << "Failed to make OpenAL context current" << std::endl;
            alcDestroyContext(context);
            alcCloseDevice(device);
            context = nullptr;
            device = nullptr;
            return;
        }

        // Global sound configs
        // Sound gets quieter as it gets farther from the listener
        alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);  // most commonly used for 3D games

        // Create a pool of AL sources, so we don't have to create every time we play a sound
        // This is common practice
        sourcePool.resize(SOURCE_POOL_SIZE);
        alGenSources(SOURCE_POOL_SIZE, sourcePool.data());
        checkALError("Generating source pool");
    }

    void AudioSystem::destroy() {
        if (!device || !context) return;

        // clear all source pools
        for (ALuint source : sourcePool) {
            alSourceStop(source);
            alSourcei(source, AL_BUFFER, AL_NONE);  // detach any buffer from the source
        }
        if (!sourcePool.empty()) {
            alDeleteSources(static_cast<ALsizei>(sourcePool.size()), sourcePool.data());
            sourcePool.clear();
        }

        // destroy context and close device
        alcMakeContextCurrent(nullptr);
        if (context) {
            alcDestroyContext(context);
            context = nullptr;
        }
        if (device) {
            alcCloseDevice(device);
            device = nullptr;
        }
    }

    void AudioSystem::update(World* world) {
        if (!context) return;

        for (Entity* entity : world->getEntities()) {
            // Update the listener position
            if (CameraComponent* camera = entity->getComponent<CameraComponent>(); camera) {
                glm::mat4 M = entity->getLocalToWorldMatrix();

                glm::vec3 listenerPos = glm::vec3(M * glm::vec4(0, 0, 0, 1));  // position from the world matrix (col 3)
                alListener3f(AL_POSITION, listenerPos.x, listenerPos.y, listenerPos.z);

                // Extract forward and up from the world matrix

                // this may need a cleanup
                glm::vec3 forward = glm::normalize(glm::vec3(M * glm::vec4(0, 0, -1, 0)));  // forward is -Z
                glm::vec3 up = glm::normalize(glm::vec3(M * glm::vec4(0, 1, 0, 0)));        // up is +Y

                float orientation[6] = {forward.x, forward.y, forward.z, up.x, up.y, up.z};
                alListenerfv(AL_ORIENTATION, orientation);
            }

            // Handle AudioSource components
            AudioSourceComponent* audioSource = entity->getComponent<AudioSourceComponent>();
            if (!audioSource) continue;

            // If this component wants to play on start but hasn't been assigned a source yet, start it
            if (audioSource->playOnStart && audioSource->alSource == 0) {
                startSource(audioSource, entity);
            }

            // Update the source's 3D position every frame (if it has a source assigned and is spatial)
            if (audioSource->alSource != 0 && audioSource->spatial) {
                glm::mat4 M = entity->getLocalToWorldMatrix();
                glm::vec3 sourcePos = M[3];  // position from the world matrix (col 3)
                alSource3f(audioSource->alSource, AL_POSITION, sourcePos.x, sourcePos.y, sourcePos.z);
            }
        }
    }

    void AudioSystem::startSource(AudioSourceComponent* audioSource, Entity* entity) {
        AudioBuffer* buffer = AssetLoader<AudioBuffer>::get(audioSource->soundName);

        if (!buffer) {
            std::cerr << "[Audio] Could not find sound buffer: " << audioSource->soundName << std::endl;
            return;
        }

        ALuint source;
        if (audioSource->spatial) {
            // Set initial position
            glm::mat4 M = entity->getLocalToWorldMatrix();
            glm::vec3 sourcePos = M[3];
            source = playSound(buffer, sourcePos, audioSource->gain, audioSource->pitch, audioSource->loop,
                               audioSource->refDistance, audioSource->maxDistance);
        } else {
            source = playSound2D(buffer, audioSource->gain, audioSource->pitch, audioSource->loop);
        }

        audioSource->alSource = source;
    }

    ALuint AudioSystem::playSound(AudioBuffer* buffer, const glm::vec3& position, float gain, float pitch, bool loop,
                                  float refDistance, float maxDistance) {
        if (!buffer) return 0;

        ALuint source = findAvailableSource();
        if (source == 0) {
            std::cerr << "No available audio sources to play sound" << std::endl;
            return 0;
        }

        // Configure the source
        alSourcei(source, AL_BUFFER, static_cast<ALint>(buffer->getBufferID()));
        alSourcef(source, AL_GAIN, gain);
        alSourcef(source, AL_PITCH, pitch);
        alSourcei(source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
        alSourcei(source, AL_SOURCE_RELATIVE, AL_FALSE);  // world-space positioning (not relative to listener)
        alSource3f(source, AL_POSITION, position.x, position.y, position.z);

        // Set attenuation defaults
        alSourcef(source, AL_REFERENCE_DISTANCE, refDistance);  // full volume within this distance
        alSourcef(source, AL_MAX_DISTANCE, maxDistance);        // Attentuation limit
        alSourcef(source, AL_ROLLOFF_FACTOR, 1.0f);             // how quickly the sound attenuates with distance

        alSourcePlay(source);
        checkALError("Playing sound");
        return source;
    }

    ALuint AudioSystem::playSound2D(AudioBuffer* buffer, float gain, float pitch, bool loop) {
        if (!buffer) return 0;

        ALuint source = findAvailableSource();
        if (source == 0) {
            std::cerr << "No available audio sources to play sound" << std::endl;
            return 0;
        }

        // Configure the source
        alSourcei(source, AL_BUFFER, static_cast<ALint>(buffer->getBufferID()));
        alSourcef(source, AL_GAIN, gain);
        alSourcef(source, AL_PITCH, pitch);
        alSourcei(source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);

        // 2D sound — relative to listener, always centered
        alSourcei(source, AL_SOURCE_RELATIVE, AL_TRUE);
        alSource3f(source, AL_POSITION, 0.0f, 0.0f, 0.0f);

        alSourcePlay(source);
        checkALError("Playing 2D sound");
        return source;
    }

    bool AudioSystem::isPlaying(ALuint source) {
        if (source == 0) return false;

        ALint state;
        alGetSourcei(source, AL_SOURCE_STATE, &state);
        return state == AL_PLAYING;
    }

    void AudioSystem::stopSound(ALuint source) {
        alSourceStop(source);
        alSourcei(source, AL_BUFFER, AL_NONE);  // detach buffer so it can be safely deleted
        checkALError("Stopping sound");
    }

    void AudioSystem::pauseAll() {
        for (ALuint source : sourcePool) {
            ALint state;
            alGetSourcei(source, AL_SOURCE_STATE, &state);
            if (state == AL_PLAYING) alSourcePause(source);
        }
        checkALError("Pausing all sounds");
    }

    void AudioSystem::resumeAll() {
        for (ALuint source : sourcePool) {
            ALint state;
            alGetSourcei(source, AL_SOURCE_STATE, &state);
            if (state == AL_PAUSED) alSourcePlay(source);
        }
        checkALError("Resuming all sounds");
    }

    void AudioSystem::stopAll() {
        for (ALuint source : sourcePool) {
            alSourceStop(source);
            alSourcei(source, AL_BUFFER, AL_NONE);  // detach buffer so it can be safely deleted
        }
        checkALError("Stopping all sounds");
    }

    void AudioSystem::setMasterVolume(float volume) {
        alListenerf(AL_GAIN, volume);
        checkALError("Setting master volume");
    }

    ALuint AudioSystem::findAvailableSource() {
        size_t poolSize = sourcePool.size();
        for (size_t i = 0; i < poolSize; ++i) {
            // Start searching from the last used index to distribute usage more evenly
            size_t index = (nextSourceIndex + i) % poolSize;
            ALuint source = sourcePool[index];

            ALint state;
            alGetSourcei(source, AL_SOURCE_STATE, &state);
            if (state == AL_STOPPED || state == AL_INITIAL) {
                nextSourceIndex = (index + 1) % poolSize;  // Update next index for the following search
                return source;
            }
        }

        ALuint hijackedSource = sourcePool[nextSourceIndex];
        ALint state;
        alGetSourcei(hijackedSource, AL_SOURCE_STATE, &state);

        if (state == AL_PLAYING) {
            alSourceStop(hijackedSource);
        }

        // Forcefully stop the old sound so OpenAL accepts the new buffer cleanly
        nextSourceIndex = (nextSourceIndex + 1) % poolSize;

        return hijackedSource;
    }

}  // namespace our
