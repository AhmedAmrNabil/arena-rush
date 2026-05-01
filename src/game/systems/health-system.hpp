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
        float healthReward = 0.0f;
        int ammoReward = 0;
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
                    our::AnimationComponent* anim = entity->getComponent<our::AnimationComponent>();
                    bool shouldRemove = (anim == nullptr);
                    float deathPercent = 0.0f;  // funny variable name xd
                    if (anim) {
                        anim->play("death", 1.0f, false);
                        shouldRemove = (anim->getState() == our::AnimationState::Death && anim->animator.isFinished());
                        if (anim->currentClip == "death") {
                            deathPercent = anim->animator.getPlaybackPercent();
                        }
                    }
                    EnemyComponent* enemy = entity->getComponent<EnemyComponent>();

                    if (enemy && enemy->type == EnemyType::Flyer) {
                        // change the height of the flyer as it dies to create a sinking effect
                        our::Transform& transform = entity->localTransform;
                        transform.position.y = enemy->baseHeight * (1 - std::min(deathPercent * 1.2f, 1.0f));
                    }

                    if (shouldRemove) {
                        if (enemy) {
                            result.kills++;
                            result.score += enemy->scoreValue;
                            // per-kill rewards
                            switch (enemy->type) {
                                case EnemyType::Brute:
                                    result.healthReward += 5.0f;
                                    result.ammoReward += 8;
                                    break;
                                case EnemyType::Charger:
                                    result.healthReward += 2.0f;
                                    result.ammoReward += 5;
                                    break;
                                case EnemyType::Flyer:
                                    result.healthReward += 8.0f;
                                    result.ammoReward += 12;
                                    break;
                            }
                        }
                        world->markForRemoval(entity);
                    }
                }
            }

            return result;
        }
    };

}  // namespace gameplay
