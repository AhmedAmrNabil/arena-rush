#pragma once

#include "../ecs/component-registry.hpp"
#include "../ecs/entity.hpp"
#include "animation.hpp"
#include "audio-source.hpp"
#include "camera.hpp"
#include "free-camera-controller.hpp"
#include "mesh-renderer.hpp"
#include "model-renderer.hpp"
#include "movement.hpp"

namespace our {

    inline void registerCommonComponents() {
        static bool initialized = false;
        if (initialized) return;

        ComponentRegistry::registerType<CameraComponent>();
        ComponentRegistry::registerType<FreeCameraControllerComponent>();
        ComponentRegistry::registerType<MovementComponent>();
        ComponentRegistry::registerType<MeshRendererComponent>();
        ComponentRegistry::registerType<AudioSourceComponent>();
        ComponentRegistry::registerType<Light>();
        ComponentRegistry::registerType<ModelRendererComponent>();
        ComponentRegistry::registerType<AnimationComponent>();
        initialized = true;
    }

    // Given a json object, this function picks and creates a component in the given entity
    // based on the "type" specified in the json object which is later deserialized from the rest of the json object
    inline void deserializeComponent(const nlohmann::json& data, Entity* entity) {
        registerCommonComponents();
        std::string type = data.value("type", "");
        Component* component = ComponentRegistry::create(type, entity);
        if (component) component->deserialize(data);
    }

}  // namespace our
