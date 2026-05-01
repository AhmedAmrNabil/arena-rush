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
        attackCooldown = data.value("attackCooldown", attackCooldown);
        attackDamage = data.value("attackDamage", attackDamage);
        jumpForce = data.value("jumpForce", jumpForce);
        gravity = data.value("gravity", gravity);
        jumpCooldown = data.value("jumpCooldown", jumpCooldown);
        preferredDistance = data.value("preferredDistance", preferredDistance);
        hoverFrequency = data.value("hoverFrequency", hoverFrequency);
        hoverAmplitude = data.value("hoverAmplitude", hoverAmplitude);
        baseHeight = data.value("baseHeight", baseHeight);
        scoreValue = data.value("scoreValue", scoreValue);
        attackSound = data.value("attackSound", attackSound);
    }

}  // namespace gameplay
