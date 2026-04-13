#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include "entity.hpp"

namespace our {

    class ComponentRegistry {
    private:
        template <typename T>
        static Component* createComponent(Entity* entity) {
            return entity->addComponent<T>();
        }

        static inline std::unordered_map<std::string, std::function<Component*(Entity*)>> factories = {};

    public:
        template <typename T>
        static void registerType() {
            factories[T::getID()] = createComponent<T>;
        }

        static Component* create(const std::string& type, Entity* entity) {
            auto it = factories.find(type);

            if (it == factories.end()) return nullptr;

            return it->second(entity);
        }
    };

}  // namespace our
