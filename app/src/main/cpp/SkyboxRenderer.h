#ifndef MY_APPLICATION_SKYBOXRENDERER_H
#define MY_APPLICATION_SKYBOXRENDERER_H

#include "Component.h"
#include "CubemapTextureAsset.h"
#include <memory>
#include <vector>

/**
 * @brief Skybox渲染组件
 *
 * 用于渲染天空盒（Skybox）的组件，使用立方体贴图作为纹理。
 * Skybox通常渲染在场景的最远处，不随相机移动而移动。
 */
class SkyboxRenderer : public Component {
public:
    /**
     * @brief Skybox顶点结构
     *
     * 只需要位置，不需要UV或颜色
     */
    struct SkyboxVertex {
        glm::vec3 position;

        SkyboxVertex(const glm::vec3& pos) : position(pos) {}
    };

    /**
     * @brief 构造函数
     * @param cubemap cubemap纹理资源
     */
    SkyboxRenderer(std::shared_ptr<CubemapTextureAsset> cubemap);

    std::shared_ptr<CubemapTextureAsset> getCubemap() const { return cubemap_; }

    /**
     * @brief 获取skybox立方体顶点数据
     */
    static const std::vector<SkyboxVertex>& getSkyboxVertices();

    /**
     * @brief 获取skybox立方体索引数据
     */
    static const std::vector<uint16_t>& getSkyboxIndices();

private:
    std::shared_ptr<CubemapTextureAsset> cubemap_;
};

#endif //MY_APPLICATION_SKYBOXRENDERER_H
