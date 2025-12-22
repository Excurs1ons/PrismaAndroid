#pragma once
#include <vector>
#include <memory>
#include "GameObject.h"

class Scene {
public:
    void addGameObject(std::shared_ptr<GameObject> go) {
        gameObjects_.push_back(go);
    }

    const std::vector<std::shared_ptr<GameObject>>& getGameObjects() const {
        return gameObjects_;
    }

    void update(float deltaTime) {
        for (auto& go : gameObjects_) {
            go->update(deltaTime);
        }
    }

private:
    std::vector<std::shared_ptr<GameObject>> gameObjects_;
};
