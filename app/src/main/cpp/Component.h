#pragma once
#include <memory>

class GameObject;

class Component {
public:
    virtual ~Component() = default;
    virtual void update(float deltaTime) {}

    void setOwner(GameObject* owner) { owner_ = owner; }
    GameObject* getOwner() const { return owner_; }

protected:
    GameObject* owner_ = nullptr;
};
