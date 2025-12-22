#pragma once
#include <vector>
#include <memory>
#include <string>
#include "Component.h"
#include "MathTypes.h"

class GameObject {
public:
    std::string name;

    void addComponent(std::shared_ptr<Component> component) {
        component->setOwner(this);
        components_.push_back(component);
    }

    template <typename T>
    std::shared_ptr<T> getComponent() {
        for (auto& comp : components_) {
            if (auto casted = std::dynamic_pointer_cast<T>(comp)) {
                return casted;
            }
        }
        return nullptr;
    }

    void update(float deltaTime) {
        for (auto& comp : components_) {
            comp->update(deltaTime);
        }
    }

    Vector3 position{0.0f, 0.0f, 0.0f};
    Vector3 rotation{0.0f, 0.0f, 0.0f}; // Euler angles in degrees
    Vector3 scale{1.0f, 1.0f, 1.0f};

    Matrix4 getTransformMatrix() {
        Matrix4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, glm::radians(rotation.x), Vector3(1, 0, 0));
        model = glm::rotate(model, glm::radians(rotation.y), Vector3(0, 1, 0));
        model = glm::rotate(model, glm::radians(rotation.z), Vector3(0, 0, 1));
        model = glm::scale(model, scale);
        return model;
    }

private:
    std::vector<std::shared_ptr<Component>> components_;
};
