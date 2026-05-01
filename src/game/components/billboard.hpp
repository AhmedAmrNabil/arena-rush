#pragma once

#include <ecs/component.hpp>

namespace gameplay {

    class BillboardComponent : public our::Component {
    public:
        static std::string getID() {
            return "Billboard";
        }

        void deserialize(const nlohmann::json& data) override {
            // Nothing to deserialize currently
        }
    };

}  // namespace gameplay
