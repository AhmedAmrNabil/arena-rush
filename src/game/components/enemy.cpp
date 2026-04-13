#include "enemy.hpp"

static gameplay::EnemyType parseEnemyType(const std::string& type, gameplay::EnemyType fallback) {
    if (type == "Brute") return gameplay::EnemyType::Brute;
    if (type == "Charger") return gameplay::EnemyType::Charger;
    if (type == "Flyer") return gameplay::EnemyType::Flyer;
    return fallback;
}

namespace gameplay {

    void EnemyComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;
        type = parseEnemyType(data.value("enemyType", std::string("Brute")), type);
        moveSpeed = data.value("moveSpeed", moveSpeed);
        acceleration = data.value("acceleration", acceleration);
        turnSpeed = data.value("turnSpeed", turnSpeed);
        aggroRange = data.value("aggroRange", aggroRange);
        attackRange = data.value("attackRange", attackRange);
        separationRadius = data.value("separationRadius", separationRadius);
        preferredDistance = data.value("preferredDistance", preferredDistance);
        scoreValue = data.value("scoreValue", scoreValue);
    }

}  // namespace gameplay
