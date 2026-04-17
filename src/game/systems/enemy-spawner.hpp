#pragma once

#include <algorithm>
#include <asset-loader.hpp>
#include <components/mesh-renderer.hpp>
#include <deserialize-utils.hpp>
#include <ecs/world.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <json/json.hpp>
#include <string>
#include <vector>

#include "../components/collider.hpp"
#include "../components/enemy.hpp"
#include "../components/health.hpp"

namespace gameplay {

    struct EnemyTemplateConfig {
        std::string name = "Enemy";
        std::string mesh = "monster";
        std::string material = "monster";
        glm::vec3 rotationDegrees = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 scale = glm::vec3(0.45f);
        nlohmann::json components = nlohmann::json::array({
            {{"type", "Enemy"},
             {"enemyType", "Brute"},
             {"moveSpeed", 2.8f},
             {"turnSpeed", 5.5f},
             {"aggroRange", 120.0f},
             {"attackRange", 2.0f},
             {"attackDamage", 10.0f},
             {"attackCooldown", 1.2f},
             {"scoreValue", 100}},
            {{"type", "Health"}, {"maxHealth", 60.0f}},
            {{"type", "Collider"}, {"layer", "enemy"}, {"radius", 1.0f}},
        });
    };

    struct EnemySpawnConfig {
        std::vector<glm::vec3> spawnPoints = {
            {30.0f, 0.0f, 30.0f}, {-30.0f, 0.0f, 30.0f}, {30.0f, 0.0f, -30.0f}, {-30.0f, 0.0f, -30.0f},
            {42.0f, 0.0f, 0.0f},  {-42.0f, 0.0f, 0.0f},  {0.0f, 0.0f, 42.0f},   {0.0f, 0.0f, -42.0f},
        };
        float spawnInterval = 2.0f;
        int initialSpawnCount = 4;
        int maxAliveEnemies = 12;
        std::vector<EnemyTemplateConfig> enemies;
    };

    class EnemySpawner {
        EnemySpawnConfig config;
        float spawnTimer = 0.0f;
        int nextSpawn = 0;
        int nextEnemy = 0;

        int countAliveEnemies(our::World* world) const {
            if (!world) return 0;

            int alive = 0;
            for (our::Entity* entity : world->getEntities()) {
                EnemyComponent* enemy = entity->getComponent<EnemyComponent>();
                if (!enemy) continue;

                HealthComponent* health = entity->getComponent<HealthComponent>();
                if (!(health && health->isDead)) alive++;
            }

            return alive;
        }

        void spawnEnemyAt(our::World* world, const EnemyTemplateConfig& config, const glm::vec3& position) const {
            if (!world) return;

            our::Mesh* mesh = our::AssetLoader<our::Mesh>::get(config.mesh);
            our::Material* material = our::AssetLoader<our::Material>::get(config.material);
            if (!(mesh && material)) return;

            our::Entity* enemyEntity = world->add();
            enemyEntity->name = config.name;
            enemyEntity->localTransform.position = position;
            enemyEntity->localTransform.rotation = glm::radians(config.rotationDegrees);
            enemyEntity->localTransform.scale = config.scale;

            our::MeshRendererComponent* rendererComponent = enemyEntity->addComponent<our::MeshRendererComponent>();
            rendererComponent->mesh = mesh;
            rendererComponent->material = material;

            for (const nlohmann::json& componentConfig : config.components) {
                if (!componentConfig.is_object()) continue;

                std::string type = componentConfig.value("type", "");
                if (type == "Enemy") {
                    EnemyComponent* enemy = enemyEntity->addComponent<EnemyComponent>();
                    enemy->deserialize(componentConfig);
                } else if (type == "Health") {
                    HealthComponent* health = enemyEntity->addComponent<HealthComponent>();
                    health->deserialize(componentConfig);
                } else if (type == "Collider") {
                    ColliderComponent* collider = enemyEntity->addComponent<ColliderComponent>();
                    collider->deserialize(componentConfig);
                }
            }
        }

        void spawnNextEnemy(our::World* world) {
            if (!world || config.spawnPoints.empty() || config.enemies.empty()) return;

            spawnEnemyAt(world, config.enemies[nextEnemy], config.spawnPoints[nextSpawn]);
            nextSpawn = (nextSpawn + 1) % config.spawnPoints.size();
            nextEnemy = (nextEnemy + 1) % config.enemies.size();
        }

    public:
        void deserialize(const nlohmann::json& sceneConfig) {
            if (!(sceneConfig.contains("game") && sceneConfig["game"].contains("enemySpawner"))) return;

            const nlohmann::json& spawnerConfig = sceneConfig["game"]["enemySpawner"];
            config.spawnInterval = spawnerConfig.value("spawnInterval", config.spawnInterval);
            config.initialSpawnCount = spawnerConfig.value("initialSpawnCount", config.initialSpawnCount);
            config.maxAliveEnemies = spawnerConfig.value("maxAliveEnemies", config.maxAliveEnemies);

            if (spawnerConfig.contains("spawnPoints") && spawnerConfig["spawnPoints"].is_array()) {
                config.spawnPoints.clear();
                for (const nlohmann::json& spawnPoint : spawnerConfig["spawnPoints"])
                    config.spawnPoints.push_back(spawnPoint.get<glm::vec3>());
            }

            if (spawnerConfig.contains("enemies") && spawnerConfig["enemies"].is_array()) {
                config.enemies.clear();
                for (const nlohmann::json& enemyConfig : spawnerConfig["enemies"]) {
                    if (!enemyConfig.is_object()) continue;

                    EnemyTemplateConfig enemyTemplate;
                    enemyTemplate.name = enemyConfig.value("name", enemyTemplate.name);
                    enemyTemplate.mesh = enemyConfig.value("mesh", enemyTemplate.mesh);
                    enemyTemplate.material = enemyConfig.value("material", enemyTemplate.material);
                    enemyTemplate.rotationDegrees = enemyConfig.value("rotation", enemyTemplate.rotationDegrees);
                    enemyTemplate.scale = enemyConfig.value("scale", enemyTemplate.scale);

                    if (enemyConfig.contains("components") && enemyConfig["components"].is_array()) {
                        enemyTemplate.components = enemyConfig["components"];
                    }

                    config.enemies.push_back(enemyTemplate);
                }
            } else {
                config.enemies = {};
            }
        }

        void initialize(our::World* world) {
            nextSpawn = 0;
            nextEnemy = 0;
            spawnTimer = config.spawnInterval;

            for (int i = 0; i < config.initialSpawnCount; i++) spawnNextEnemy(world);
        }

        void update(our::World* world, float diffTime) {
            if (!world || diffTime <= 0.0f) return;

            spawnTimer -= diffTime;
            if (spawnTimer > 0.0f) return;

            if (countAliveEnemies(world) >= config.maxAliveEnemies) {
                spawnTimer = 0.5f;
                return;
            }

            spawnNextEnemy(world);
            spawnTimer = config.spawnInterval;
        }
    };

}  // namespace gameplay
