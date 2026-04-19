#pragma once

#include <components/post-process-effects.hpp>
#include <ecs/world.hpp>
#include <glm/common.hpp>

#include "../components/player-movement.hpp"

namespace gameplay {

    class PostProcessEffectsSystem {
        float dashEffectTimer = 0.0f;
        static constexpr float DASH_EFFECT_DURATION = 0.3f;

    public:
        void update(our::World* world, float deltaTime) {
            if (!world || deltaTime <= 0.0f) return;

            PlayerMovementComponent* movement = nullptr;
            our::PostProcessEffectsComponent* effects = nullptr;

            for (our::Entity* entity : world->getEntities()) {
                if (!movement) movement = entity->getComponent<PlayerMovementComponent>();
                if (!effects) effects = entity->getComponent<our::PostProcessEffectsComponent>();
                if (movement && effects) break;
            }

            if (!movement || !effects) return;

            if (movement->dashTriggeredThisFrame) {
                dashEffectTimer = DASH_EFFECT_DURATION;
            }

            if (dashEffectTimer > 0.0f) {
                dashEffectTimer = glm::max(0.0f, dashEffectTimer - deltaTime);
            }

            effects->uniforms["intensity"] = glm::max(0.0f, dashEffectTimer / DASH_EFFECT_DURATION);
        }
    };

}  // namespace gameplay
