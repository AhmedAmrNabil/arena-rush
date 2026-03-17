#include "movement.hpp"

#include "../deserialize-utils.hpp"
#include "../ecs/entity.hpp"

namespace our {
    // Reads linearVelocity & angularVelocity from the given json object
    void MovementComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;
        linearVelocity = data.value("linearVelocity", linearVelocity);
        angularVelocity = glm::radians(data.value("angularVelocity", angularVelocity));
    }
}  // namespace our