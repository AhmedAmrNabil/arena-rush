#pragma once

#include <algorithm>
#include <ecs/world.hpp>

#include "../components/health.hpp"
#include "../components/player.hpp"

namespace gameplay {
    class HealthSystem {
    public:
        bool update(our::World* world, float deltaTime) {
            bool result = false;
            if (!world) return result;

            for (our::Entity* entity : world->getEntities()) {
                HealthComponent* health = entity->getComponent<HealthComponent>();
                if (!health) continue;

                if (deltaTime > 0.0f && health->damageRevealTimer > 0.0f)
                    health->damageRevealTimer = std::max(0.0f, health->damageRevealTimer - deltaTime);

                if (!health->isDead) continue;

                if (entity->getComponent<PlayerComponent>())
                    result = true;
                else
                    world->markForRemoval(entity);
            }

            return result;
        }
    };

}  // namespace gameplay
