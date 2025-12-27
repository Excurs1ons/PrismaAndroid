#ifndef MY_APPLICATION_RENDERERVULKAN_H
#define MY_APPLICATION_RENDERERVULKAN_H

#include "RendererAPI.h"
#include "VulkanContext.h"
#include "Scene.h"
#include "renderer/RenderPipeline.h"
#include <memory>

// 前向声明
class BackgroundPass;
class OpaquePass;

struct android_app;

class RendererVulkan : public RendererAPI {
public:
    RendererVulkan(android_app *pApp);
    virtual ~RendererVulkan();

    void init() override;
    void render() override;
    void onConfigChanged() override;  // 处理屏幕旋转

    // SwapChain 重建相关函数（处理屏幕旋转）
    void cleanupSwapChain();      // 清理旧的 SwapChain 相关资源
    void recreateSwapChain();     // 重建 SwapChain 以适应新的屏幕尺寸

private:
    // === 初始化方法 ===
    void createScene();
    void createRenderPipeline();     // 创建逻辑渲染管线（封装 Pass）
    void createFramebuffers();
    void createCommandPool();
    void createTextureImage();
    void createTextureImageView();
    void createTextureSampler();
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSetLayout();  // 创建描述符集布局（需要在 allocate descriptor sets 之前调用）
    void createDescriptorSets();
    void createSkyboxDescriptorSets();
    void createCommandBuffers();
    void createSyncObjects();

    // === 渲染方法 ===
    void updateUniformBuffer(uint32_t currentImage);
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    android_app *app_;
    VulkanContext vulkanContext_;
    std::unique_ptr<Scene> scene_;

    const int MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t currentFrame = 0;





    // === 逻辑渲染管线 ===
    std::unique_ptr<RenderPipeline> renderPipeline_;

    // === 临时数据（初始化后会移动到 Pass 中） ===
    std::vector<RenderObjectData> renderObjects;
    SkyboxRenderData skyboxData_;
    ClearColorData clearColorData_;

    // Vulkan resources for the scene
    // （Pipeline 创建已迁移到 Pass，但 descriptorSetLayout 仍保留用于创建 descriptor sets）
    VkDescriptorSetLayout descriptorSetLayout;        // MeshRenderer 用的 descriptor set layout
    VkDescriptorSetLayout skyboxDescriptorSetLayout;  // Skybox 用的 descriptor set layout

    VkDescriptorPool descriptorPool;

};

#endif //MY_APPLICATION_RENDERERVULKAN_H