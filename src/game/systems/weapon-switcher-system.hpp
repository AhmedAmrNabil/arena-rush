#pragma once

#include <application.hpp>
#include <asset-loader.hpp>
#include <components/weapon.hpp>
#include <ecs/world.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace gameplay {

    class WeaponSwitcherSystem {
    public:
        std::vector<WeaponComponent*> weapons;
        our::Entity* playerEntity = nullptr;

        void initialize(our::World* world, our::Entity* playerEntity) {
            this->playerEntity = playerEntity;
            for (our::Entity* entity : world->getEntities()) {
                WeaponComponent* weapon = entity->getComponent<WeaponComponent>();
                if (weapon && weapon->getOwner()->parent == playerEntity) {
                    weapons.push_back(weapon);
                }
            }

            std::sort(weapons.begin(), weapons.end(),
                      [](WeaponComponent* a, WeaponComponent* b) { return a->order < b->order; });
        }

        void update(our::Application* app) {
            if (!app || weapons.empty() || !playerEntity) return;

            PlayerComponent* playerComp = playerEntity->getComponent<PlayerComponent>();
            if (!playerComp) return;

            int currentIndex = playerComp->currentActiveWeaponIndex;

            our::Keyboard& keyboard = app->getKeyboard();
            for (int i = 0; i < (int)weapons.size(); i++) {
                if (keyboard.justPressed(GLFW_KEY_1 + i)) {
                    playerComp->currentActiveWeaponIndex = i;
                    break;
                }
            }

            // handle scroll wheel with wrapping
            our::Mouse& mouse = app->getMouse();
            if (mouse.getScrollOffset().y > 0.0f) {
                playerComp->currentActiveWeaponIndex = (playerComp->currentActiveWeaponIndex + 1) % weapons.size();
            } else if (mouse.getScrollOffset().y < 0.0f) {
                playerComp->currentActiveWeaponIndex =
                    (playerComp->currentActiveWeaponIndex - 1 + weapons.size()) % weapons.size();
            }

            weapons[currentIndex]->isActive = false;
            weapons[playerComp->currentActiveWeaponIndex]->isActive = true;

            playerComp->currentWeapon = weapons[playerComp->currentActiveWeaponIndex];
        }

        void destroy() {
            weapons.clear();
            playerEntity = nullptr;
        }
    };
}  // namespace gameplay
