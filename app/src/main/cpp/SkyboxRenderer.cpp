#include "SkyboxRenderer.h"
#include <glm/glm.hpp>

SkyboxRenderer::SkyboxRenderer(std::shared_ptr<CubemapTextureAsset> cubemap)
    : cubemap_(cubemap) {}

const std::vector<SkyboxRenderer::SkyboxVertex>& SkyboxRenderer::getSkyboxVertices() {
    static const std::vector<SkyboxVertex> vertices = {
        // Front
        SkyboxVertex(glm::vec3(-1.0f, -1.0f,  1.0f)),
        SkyboxVertex(glm::vec3( 1.0f, -1.0f,  1.0f)),
        SkyboxVertex(glm::vec3( 1.0f,  1.0f,  1.0f)),
        SkyboxVertex(glm::vec3(-1.0f,  1.0f,  1.0f)),
        // Back
        SkyboxVertex(glm::vec3( 1.0f, -1.0f, -1.0f)),
        SkyboxVertex(glm::vec3(-1.0f, -1.0f, -1.0f)),
        SkyboxVertex(glm::vec3(-1.0f,  1.0f, -1.0f)),
        SkyboxVertex(glm::vec3( 1.0f,  1.0f, -1.0f)),
        // Top
        SkyboxVertex(glm::vec3(-1.0f,  1.0f, -1.0f)),
        SkyboxVertex(glm::vec3(-1.0f,  1.0f,  1.0f)),
        SkyboxVertex(glm::vec3( 1.0f,  1.0f,  1.0f)),
        SkyboxVertex(glm::vec3( 1.0f,  1.0f, -1.0f)),
        // Bottom
        SkyboxVertex(glm::vec3(-1.0f, -1.0f, -1.0f)),
        SkyboxVertex(glm::vec3( 1.0f, -1.0f, -1.0f)),
        SkyboxVertex(glm::vec3( 1.0f, -1.0f,  1.0f)),
        SkyboxVertex(glm::vec3(-1.0f, -1.0f,  1.0f)),
        // Right
        SkyboxVertex(glm::vec3( 1.0f, -1.0f, -1.0f)),
        SkyboxVertex(glm::vec3( 1.0f, -1.0f,  1.0f)),
        SkyboxVertex(glm::vec3( 1.0f,  1.0f,  1.0f)),
        SkyboxVertex(glm::vec3( 1.0f,  1.0f, -1.0f)),
        // Left
        SkyboxVertex(glm::vec3(-1.0f, -1.0f,  1.0f)),
        SkyboxVertex(glm::vec3(-1.0f, -1.0f, -1.0f)),
        SkyboxVertex(glm::vec3(-1.0f,  1.0f, -1.0f)),
        SkyboxVertex(glm::vec3(-1.0f,  1.0f,  1.0f)),
    };
    return vertices;
}

const std::vector<uint16_t>& SkyboxRenderer::getSkyboxIndices() {
    static const std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0,       // Front
        4, 5, 6, 6, 7, 4,       // Back
        8, 9, 10, 10, 11, 8,    // Top
        12, 13, 14, 14, 15, 12, // Bottom
        16, 17, 18, 18, 19, 16, // Right
        20, 21, 22, 22, 23, 20  // Left
    };
    return indices;
}
