#ifndef ANDROIDGLINVESTIGATIONS_MODEL_H
#define ANDROIDGLINVESTIGATIONS_MODEL_H

#include <vector>
#include "TextureAsset.h"
#include "MathTypes.h"

struct Vertex {
    Vertex(const Vector3 &inPosition, const Vector3 &inColor, const Vector2 &inUV) 
        : position(inPosition), color(inColor), uv(inUV) {}

    Vector3 position;
    Vector3 color;
    Vector2 uv;
};

typedef uint16_t Index;

class Model {
public:
    inline Model(
            std::vector<Vertex> vertices,
            std::vector<Index> indices,
            std::shared_ptr<TextureAsset> spTexture)
            : vertices_(std::move(vertices)),
              indices_(std::move(indices)),
              spTexture_(std::move(spTexture)) {}

    inline const Vertex *getVertexData() const {
        return vertices_.data();
    }

    inline const size_t getVertexCount() const {
        return vertices_.size();
    }

    inline const size_t getIndexCount() const {
        return indices_.size();
    }

    inline const Index *getIndexData() const {
        return indices_.data();
    }

    inline const TextureAsset &getTexture() const {
        return *spTexture_;
    }

private:
    std::vector<Vertex> vertices_;
    std::vector<Index> indices_;
    std::shared_ptr<TextureAsset> spTexture_;
};

#endif //ANDROIDGLINVESTIGATIONS_MODEL_H