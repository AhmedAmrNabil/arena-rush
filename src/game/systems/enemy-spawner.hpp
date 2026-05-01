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

#include "components/animation.hpp"
#include "components/audio-source.hpp"
#include "components/collider.hpp"
#include "components/enemy.hpp"
#include "components/health.hpp"
#include "components/weapon.hpp"

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
        float countdownDuration = 4.0f;
    };

    class EnemySpawner {
    public:
        enum class WaveState { Countdown, Active };

    private:
        EnemySpawnConfig config;
        std::vector<WaveConfig> waves;
        WaveReward waveReward;

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

        // resolve an enemy name to its index in config.enemies, returns 0 if not found
        int resolveEnemyIndex(const std::string& name) const {
            for (int i = 0; i < (int)config.enemies.size(); i++) {
                if (config.enemies[i].name == name) return i;
            }
            return 0;
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
                if (type == EnemyComponent::getID()) {
                    EnemyComponent* enemy = enemyEntity->addComponent<EnemyComponent>();

                    enemy->deserialize(componentConfig);
                    enemy->moveSpeed *= wave.speedMul;
                    enemy->attackDamage *= wave.damageMul;
                } else if (type == HealthComponent::getID()) {
                    HealthComponent* health = enemyEntity->addComponent<HealthComponent>();

                    health->deserialize(componentConfig);
                    health->maxHealth *= wave.healthMul;
                    health->currentHealth = health->maxHealth;
                } else if (type == ColliderComponent::getID()) {
                    ColliderComponent* collider = enemyEntity->addComponent<ColliderComponent>();

                    collider->deserialize(componentConfig);
                    collider->shapeCacheId = std::string("enemy:") + config.name + ":" + config.model;
                } else if (type == WeaponComponent::getID()) {
                    WeaponComponent* weapon = enemyEntity->addComponent<WeaponComponent>();

                    weapon->deserialize(componentConfig);
                    weapon->bulletDamage *= wave.damageMul;
                } else if (type == our::AnimationComponent::getID()) {
                    our::AnimationComponent* animation = enemyEntity->addComponent<our::AnimationComponent>();
                    animation->deserialize(componentConfig);
                } else if (type == our::AudioSourceComponent::getID()) {
                    our::AudioSourceComponent* audioSource = enemyEntity->addComponent<our::AudioSourceComponent>();
                    audioSource->deserialize(componentConfig);
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

            config.countdownDuration = spawnerConfig.value("countdownDuration", config.countdownDuration);

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

            // Deserialize waves
            if (spawnerConfig.contains("waves") && spawnerConfig["waves"].is_array()) {
                waves.clear();
                for (const nlohmann::json& waveJson : spawnerConfig["waves"]) {
                    if (!waveJson.is_object()) continue;

                    WaveConfig wave;
                    wave.spawnInterval = waveJson.value("spawnInterval", wave.spawnInterval);
                    wave.maxAliveEnemies = waveJson.value("maxAliveEnemies", wave.maxAliveEnemies);
                    wave.healthMul = waveJson.value("healthMul", wave.healthMul);
                    wave.damageMul = waveJson.value("damageMul", wave.damageMul);
                    wave.speedMul = waveJson.value("speedMul", wave.speedMul);

                    if (waveJson.contains("entries") && waveJson["entries"].is_array()) {
                        for (const nlohmann::json& entryJson : waveJson["entries"]) {
                            if (!entryJson.is_object()) continue;

                            WaveEntry entry;
                            entry.count = entryJson.value("count", 0);

                            // resolve enemy by name, fall back to index
                            if (entryJson.contains("enemy") && entryJson["enemy"].is_string()) {
                                entry.templateIndex = resolveEnemyIndex(entryJson["enemy"].get<std::string>());
                            } else {
                                entry.templateIndex = entryJson.value("templateIndex", 0);
                            }

                            wave.entries.push_back(entry);
                        }
                    }

                    waves.push_back(wave);
                }
            }

            // Deserialize wave reward
            if (spawnerConfig.contains("waveReward") && spawnerConfig["waveReward"].is_object()) {
                const nlohmann::json& rewardJson = spawnerConfig["waveReward"];
                waveReward.health = rewardJson.value("health", waveReward.health);
                waveReward.ammo = rewardJson.value("ammo", waveReward.ammo);
            }
        }

        void initialize(our::World* world) {
            // ensure at least one fallback wave if none were deserialized
            if (waves.empty()) {
                WaveConfig fallback;
                fallback.entries = {{0, 4}};
                waves.push_back(fallback);
            }
            currentWave = 0;
            waveDisplayNumber = 1;
            waveState = WaveState::Countdown;
            countdownTimer = config.countdownDuration;
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
                countdownTimer = config.countdownDuration;
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
            return waveReward;
        }
    };

}  // namespace gameplay
