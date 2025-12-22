#pragma once
#include "Component.h"
#include "Model.h"
#include <memory>

class MeshRenderer : public Component {
public:
    MeshRenderer(std::shared_ptr<Model> model) : model_(model) {}

    std::shared_ptr<Model> getModel() const { return model_; }

private:
    std::shared_ptr<Model> model_;
};
