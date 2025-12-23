#ifndef MY_APPLICATION_CUBEMAPTEXTUREASSET_H
#define MY_APPLICATION_CUBEMAPTEXTUREASSET_H

#include "TextureAsset.h"
#include <memory>
#include <string>
#include <vector>

class VulkanContext;

/**
 * @brief Cubemap纹理资源类
 *
 * 继承自TextureAsset，用于加载和管理立方体贴图（Cubemap）纹理，主要用于Skybox渲染。
 * 支持6个面的纹理加载（+X, -X, +Y, -Y, +Z, -Z）
 */
class CubemapTextureAsset : public TextureAsset {
public:
    /**
     * @brief 从资源加载cubemap纹理
     *
     * @param assetManager Android资源管理器
     * @param facePaths 6个面的纹理路径，顺序为：+X, -X, +Y, -Y, +Z, -Z
     * @param vulkanContext Vulkan上下文
     * @return std::shared_ptr<CubemapTextureAsset> cubemap纹理对象
     */
    static std::shared_ptr<CubemapTextureAsset> loadFromAssets(
            AAssetManager* assetManager,
            const std::vector<std::string>& facePaths,
            VulkanContext* vulkanContext);

    ~CubemapTextureAsset() override;

    // 获取cubemap特有的属性
    VkImage getImage() const { return image_; }

private:
    CubemapTextureAsset(VulkanContext* context);
};

#endif //MY_APPLICATION_CUBEMAPTEXTUREASSET_H
