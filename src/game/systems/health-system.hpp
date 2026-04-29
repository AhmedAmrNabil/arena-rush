#pragma once

#include <algorithm>
#include <ecs/world.hpp>

#include "../components/enemy.hpp"
#include "../components/health.hpp"
#include "../components/player.hpp"

namespace gameplay {

    struct HealthUpdateResult {
        bool playerDied = false;
        int kills = 0;
        int score = 0;
    };

    class HealthSystem {
    public:
        HealthUpdateResult update(our::World* world, float deltaTime) {
            HealthUpdateResult result;
            if (!world) return result;

            for (our::Entity* entity : world->getEntities()) {
                HealthComponent* health = entity->getComponent<HealthComponent>();
                if (!health) continue;

                if (deltaTime > 0.0f && health->currentDamageRevealTimer > 0.0f)
                    health->currentDamageRevealTimer = std::max(0.0f, health->currentDamageRevealTimer - deltaTime);

                if (!health->isDead) continue;

                if (entity->getComponent<PlayerComponent>()) {
                    result.playerDied = true;
                } else {
                    EnemyComponent* enemy = entity->getComponent<EnemyComponent>();
                    if (enemy) {
                        result.kills++;
                        result.score += enemy->scoreValue;
                    }
                    world->markForRemoval(entity);
                }
            }

            return result;
        }
    };

}  // namespace gameplay
