#pragma once

#include <algorithm>
#include <asset-loader.hpp>
#include <components/model-renderer.hpp>
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
#include "../components/weapon.hpp"

namespace gameplay {

    struct EnemyTemplateConfig {
        std::string name = "Enemy";
        std::string model = "monster";
        glm::vec3 rotationDegrees = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 scale = glm::vec3(0.012f);
        nlohmann::json components = nlohmann::json::array();
    };

    struct WaveEntry {
        int templateIndex;  // index of the enemy template in the EnemySpawnerConfig's enemies vector
        int count;
    };

    struct WaveConfig {
        std::vector<WaveEntry> entries;
        float spawnInterval = 2.0f;
        int maxAliveEnemies = 12;
        float healthMul = 1.0f;
        float damageMul = 1.0f;
        float speedMul = 1.0f;
    };

    struct WaveReward {
        float health = 10.0f;
        int ammo = 20;
    };

    struct EnemySpawnConfig {
        std::vector<glm::vec3> spawnPoints = {
            {30.0f, 0.0f, 30.0f}, {-30.0f, 0.0f, 30.0f}, {30.0f, 0.0f, -30.0f}, {-30.0f, 0.0f, -30.0f},
            {42.0f, 0.0f, 0.0f},  {-42.0f, 0.0f, 0.0f},  {0.0f, 0.0f, 42.0f},   {0.0f, 0.0f, -42.0f},
        };
        std::vector<EnemyTemplateConfig> enemies;
    };

    class EnemySpawner {
    public:
        enum class WaveState { Countdown, Active };

    private:
        EnemySpawnConfig config;
        std::vector<WaveConfig> waves;

        int currentWave = 0;
        int waveDisplayNumber = 1;
        WaveState waveState = WaveState::Countdown;
        float countdownTimer = 4.0f;

        int enemiesToSpawn = 0;  // remaining to spawn this wave
        int enemiesSpawned = 0;  // total spawned so far
        int totalInWave = 0;     // total for the wave
        float spawnTimer = 0.0f;
        int nextSpawnPoint = 0;
        int spawnQueueIndex = 0;      // index into the flattened queue
        std::vector<int> spawnQueue;  // queue of enemy template indices to spawn for the current wave

        bool waveJustCompleted = false;

        // utility to build the wave table
        void buildWaveTable() {
            int B = 0;
            int C = std::min(1, (int)config.enemies.size() - 1);
            int F = std::min(2, (int)config.enemies.size() - 1);

            waves.clear();

            // interval, maxAlive, hpMax, dmgMax, speedMax
            waves.push_back({{{B, 4}}, 3.0f, 4, 1.0f, 1.0f, 1.0f});
            waves.push_back({{{B, 5}, {C, 1}}, 2.5f, 5, 1.0f, 1.0f, 1.0f});
            waves.push_back({{{B, 5}, {C, 2}, {F, 1}}, 2.5f, 6, 1.15f, 1.1f, 1.05f});
            waves.push_back({{{B, 5}, {C, 3}, {F, 2}}, 2.0f, 7, 1.3f, 1.2f, 1.1f});
            waves.push_back({{{B, 5}, {C, 4}, {F, 3}}, 2.0f, 8, 1.5f, 1.3f, 1.15f});
            waves.push_back({{{B, 6}, {C, 4}, {F, 4}}, 1.8f, 9, 1.7f, 1.4f, 1.2f});
            waves.push_back({{{B, 6}, {C, 5}, {F, 5}}, 1.5f, 10, 1.9f, 1.5f, 1.25f});
            waves.push_back({{{B, 7}, {C, 5}, {F, 6}}, 1.5f, 11, 2.2f, 1.6f, 1.3f});
            waves.push_back({{{B, 7}, {C, 6}, {F, 7}}, 1.2f, 12, 2.5f, 1.8f, 1.35f});
            waves.push_back({{{B, 8}, {C, 8}, {F, 8}}, 1.0f, 14, 3.0f, 2.0f, 1.4f});
        }

        int countAliveEnemies(our::World* world) const {
            if (!world) return 0;

            int alive = 0;
            for (our::Entity* entity : world->getEntities()) {
                EnemyComponent* enemy = entity->getComponent<EnemyComponent>();
                if (!enemy) continue;

                HealthComponent* health = entity->getComponent<HealthComponent>();
                if (health && !health->isDead) alive++;
            }

            return alive;
        }

        void spawnEnemyAt(our::World* world, const EnemyTemplateConfig& config, const glm::vec3& position,
                          const WaveConfig& wave) const {
            if (!world) return;

            our::Model* model = our::AssetLoader<our::Model>::get(config.model);

            our::Entity* enemyEntity = world->add();
            enemyEntity->name = config.name;
            enemyEntity->localTransform.position = position;
            enemyEntity->localTransform.rotation = glm::radians(config.rotationDegrees);
            enemyEntity->localTransform.scale = config.scale;

            our::ModelRendererComponent* rendererComponent = enemyEntity->addComponent<our::ModelRendererComponent>();
            rendererComponent->model = model;

            for (const nlohmann::json& componentConfig : config.components) {
                if (!componentConfig.is_object()) continue;

                std::string type = componentConfig.value("type", "");
                if (type == "Enemy") {
                    EnemyComponent* enemy = enemyEntity->addComponent<EnemyComponent>();

                    enemy->deserialize(componentConfig);
                    enemy->moveSpeed *= wave.speedMul;
                    enemy->attackDamage *= wave.damageMul;
                } else if (type == "Health") {
                    HealthComponent* health = enemyEntity->addComponent<HealthComponent>();

                    health->deserialize(componentConfig);
                    health->maxHealth *= wave.healthMul;
                    health->currentHealth = health->maxHealth;
                } else if (type == "Collider") {
                    ColliderComponent* collider = enemyEntity->addComponent<ColliderComponent>();

                    collider->deserialize(componentConfig);
                    collider->shapeCacheId = std::string("enemy:") + config.name + ":" + config.model;
                } else if (type == "Weapon") {
                    WeaponComponent* weapon = enemyEntity->addComponent<WeaponComponent>();

                    weapon->deserialize(componentConfig);
                    weapon->bulletDamage *= wave.damageMul;
                }
            }
        }

        void buildSpawnQueue(const WaveConfig& wave) {
            spawnQueue.clear();

            for (const WaveEntry& entry : wave.entries) {
                for (int i = 0; i < entry.count; i++) spawnQueue.push_back(entry.templateIndex);
            }
            totalInWave = (int)spawnQueue.size();
            enemiesToSpawn = totalInWave;
            enemiesSpawned = 0;
            spawnQueueIndex = 0;
        }

        void startWave(our::World* world) {
            const WaveConfig& wave = waves[std::min(currentWave, (int)waves.size() - 1)];
            buildSpawnQueue(wave);

            spawnTimer = 0.0f;  // first enemy should spawn immediately
            waveState = WaveState::Active;
            waveJustCompleted = false;
        }

    public:
        void deserialize(const nlohmann::json& sceneConfig) {
            if (!(sceneConfig.contains("game") && sceneConfig["game"].contains("enemySpawner"))) return;

            const nlohmann::json& spawnerConfig = sceneConfig["game"]["enemySpawner"];

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
                    enemyTemplate.model = enemyConfig.value("model", enemyTemplate.model);
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
            buildWaveTable();
            currentWave = 0;
            waveDisplayNumber = 1;
            waveState = WaveState::Countdown;
            countdownTimer = 4.0f;
            nextSpawnPoint = 0;
            waveJustCompleted = false;
        }

        // returns true if a wave was just completed this frame, false otherwise
        bool update(our::World* world, float dt) {
            if (!world || dt <= 0.0f || config.spawnPoints.empty() || config.enemies.empty()) return false;

            waveJustCompleted = false;
            int waveIndex = std::min(currentWave, (int)waves.size() - 1);
            const WaveConfig& wave = waves[waveIndex];

            if (waveState == WaveState::Countdown) {
                countdownTimer -= dt;
                if (countdownTimer <= 0.0f) {
                    startWave(world);
                }
                return false;
            }

            // Case Active
            // If there are still enemies to spawn, spawn them according to the timer and maxAliveEnemies limit
            if (enemiesToSpawn > 0) {
                spawnTimer -= dt;
                if (spawnTimer <= 0.0f && countAliveEnemies(world) < wave.maxAliveEnemies) {
                    int idx = spawnQueue[spawnQueueIndex];
                    idx = std::min(idx, (int)config.enemies.size() - 1);
                    spawnEnemyAt(world, config.enemies[idx], config.spawnPoints[nextSpawnPoint], wave);

                    nextSpawnPoint = (nextSpawnPoint + 1) % config.spawnPoints.size();
                    spawnQueueIndex++;
                    enemiesToSpawn--;
                    enemiesSpawned++;
                    spawnTimer = wave.spawnInterval;
                }
            }

            // no more enemies to spawn and no more alive enemies means wave completed
            if (enemiesToSpawn <= 0 && countAliveEnemies(world) == 0) {
                waveJustCompleted = true;

                // go next wave
                currentWave++;
                waveDisplayNumber++;
                countdownTimer = 4.0f;  // countdown to next wave
                waveState = WaveState::Countdown;
            }
            return waveJustCompleted;
        }

        // for HUD
        int getWaveDisplayNumber() const {
            return waveDisplayNumber;
        }

        WaveState getWaveState() const {
            return waveState;
        }

        float getCountdownTimer() const {
            return countdownTimer;
        }

        int getEnemiesRemaining(our::World* world) const {
            if (!world) return 0;

            int alive = countAliveEnemies(world);
            return alive + enemiesToSpawn;
        }

        WaveReward getWaveReward() const {
            return {10.0f, 20};
        }
    };

}  // namespace gameplay
