#include "game-registration.hpp"

#include <ecs/component-registry.hpp>

#include "components/collider.hpp"
#include "components/enemy.hpp"
#include "components/health.hpp"
#include "components/player-movement.hpp"
#include "components/player.hpp"

namespace gameplay {

    void registerGameComponents() {
        static bool initialized = false;
        if (initialized) return;

        our::ComponentRegistry::registerType<ColliderComponent>();
        our::ComponentRegistry::registerType<EnemyComponent>();
        our::ComponentRegistry::registerType<HealthComponent>();
        our::ComponentRegistry::registerType<PlayerComponent>();
        our::ComponentRegistry::registerType<PlayerMovementComponent>();

        initialized = true;
    }

}  // namespace gameplay
