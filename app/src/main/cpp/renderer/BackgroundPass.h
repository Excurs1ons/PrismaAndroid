#ifndef PRISMA_ANDROID_BACKGROUND_PASS_H
#define PRISMA_ANDROID_BACKGROUND_PASS_H

#include "RenderPass.h"
#include <vulkan/vulkan.h>
#include <vector>
#include "../MathTypes.h"
// 前向声明
struct android_app;
class SkyboxRenderer;

// Skybox专用的UBO（不需要model矩阵）
struct SkyboxUniformBufferObject {
    alignas(16) Matrix4 view;
    alignas(16) Matrix4 proj;
};
// Skybox渲染数据
struct SkyboxRenderData {
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;
    bool hasTexture = false;  // 是否有有效的cubemap纹理
};

// 纯色渲染数据（skybox纹理为空时使用）
struct ClearColorData {
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
};


/**
 * 背景渲染通道
 *
 * 渲染天空盒或纯色背景
 *
 * 特点：
 * - 优先使用 Skybox（cubemap 纹理）
 * - 如果没有 Skybox，使用纯色填充
 * - 作为第一个渲染通道（在所有物体之前）
 *
 * 等价关系：
 * BackgroundPass ≈ skyboxPipeline + clearColorPipeline +
 *                 SkyboxRenderData + ClearColorData + 渲染命令
 *
 * API 切换注意事项：
 * - Skybox 的立方体纹理在不同 API 中有不同表示方式：
 *   Vulkan: VkImageView + cubemap layers
 *   DirectX 12: 立方体纹理资源
 *   Metal: MTLTextureTypeCube
 */
class BackgroundPass : public RenderPass {
public:
    BackgroundPass():RenderPass("Background Pass"){};

    ~BackgroundPass() override = default;

    // 禁止拷贝
    BackgroundPass(const BackgroundPass&) = delete;
    BackgroundPass& operator=(const BackgroundPass&) = delete;

    // 允许移动
    BackgroundPass(BackgroundPass&&) = default;
    BackgroundPass& operator=(BackgroundPass&&) = default;

    /**
     * 初始化背景渲染通道
     * 创建 skybox pipeline 和 clear color pipeline
     */
    void initialize(VkDevice device, VkRenderPass renderPass) override;

    /**
     * 记录渲染命令
     * 根据 hasTexture 选择渲染 Skybox 或 ClearColor
     */
    void record(VkCommandBuffer cmdBuffer) override;

    /**
     * 清理资源
     */
    void cleanup(VkDevice device) override;

    // === 数据设置方法 ===

    /**
     * 设置 Skybox 数据（转移所有权）
     * @param data Skybox 渲染数据
     */
    void setSkyboxData(const SkyboxRenderData& data);

    /**
     * 设置 ClearColor 数据（转移所有权）
     * @param data 纯色渲染数据
     */
    void setClearColorData(const ClearColorData& data);

    /**
     * 设置当前帧索引（用于选择正确的 uniform buffer）
     * @param currentFrame 当前帧索引
     */
    void setCurrentFrame(uint32_t currentFrame);

    /**
     * 设置 SwapChain 扩展信息（用于 viewport/scissor）
     */
    void setSwapChainExtent(VkExtent2D extent);

    /**
     * 设置 android_app（用于加载着色器）
     */
    void setAndroidApp(android_app* app);

    /**
     * 设置当前渲染变换（用于 viewport 计算）
     */
    void setCurrentTransform(VkSurfaceTransformFlagBitsKHR transform);


private:
    /**
     * 创建 skybox pipeline
     * 从 RendererVulkan::createSkyboxPipeline() 迁移
     */
    void createSkyboxPipeline(VkDevice device, VkRenderPass renderPass);

    /**
     * 创建 clear color pipeline
     * 从 RendererVulkan::createClearColorPipeline() 迁移
     */
    void createClearColorPipeline(VkDevice device, VkRenderPass renderPass);

    // === 数据成员 ===

    SkyboxRenderData skyboxData_;      // 持有的 Skybox 数据
    ClearColorData clearColorData_;    // 持有的纯色渲染数据

    uint32_t currentFrame_ = 0;  // 当前帧索引

    // 配置信息
    VkExtent2D swapChainExtent_{};           // SwapChain 扩展
    android_app* app_ = nullptr;             // android_app（用于加载着色器）
    VkSurfaceTransformFlagBitsKHR currentTransform_ = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;  // 当前变换（用于计算宽高比）
};

#endif //PRISMA_ANDROID_BACKGROUND_PASS_H
