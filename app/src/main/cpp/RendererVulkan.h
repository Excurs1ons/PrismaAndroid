#ifndef MY_APPLICATION_RENDERERVULKAN_H
#define MY_APPLICATION_RENDERERVULKAN_H

#include "RendererAPI.h"
#include "VulkanContext.h"
#include "Scene.h"

struct android_app;

class RendererVulkan : public RendererAPI {
public:
    RendererVulkan(android_app *pApp);
    virtual ~RendererVulkan();

    void init() override;
    void render() override;

    // SwapChain 重建相关函数（处理屏幕旋转）
    void cleanupSwapChain();      // 清理旧的 SwapChain 相关资源
    void recreateSwapChain();     // 重建 SwapChain 以适应新的屏幕尺寸

private:
    void createScene();
    void createGraphicsPipeline();
    void createSkyboxPipeline();  // 创建Skybox渲染管线
    void createClearColorPipeline();  // 创建纯色渲染管线（skybox纹理为空时使用）
    void createFramebuffers();
    void createCommandPool();
    void createTextureImage();
    void createTextureImageView();
    void createTextureSampler();
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void createSkyboxDescriptorSets();  // 创建Skybox描述符集
    void createCommandBuffers();
    void createSyncObjects();
    
    void updateUniformBuffer(uint32_t currentImage);
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    android_app *app_;
    VulkanContext vulkanContext_;
    std::unique_ptr<Scene> scene_;
    
    const int MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t currentFrame = 0;

    struct RenderObjectData {
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;
        VkBuffer indexBuffer;
        VkDeviceMemory indexBufferMemory;

        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;
        std::vector<void*> uniformBuffersMapped;
        std::vector<VkDescriptorSet> descriptorSets;
    };

    std::vector<RenderObjectData> renderObjects;

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
    } skyboxData_;

    // 纯色渲染数据（skybox纹理为空时使用）
    struct ClearColorData {
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    } clearColorData_;

    // Vulkan resources for the scene
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkDescriptorPool descriptorPool;

    // Texture resources are managed by TextureAsset, but we might need to keep track if we want to support multiple textures later.
    // For now, we assume TextureAsset handles creation/destruction of image/view/sampler, 
    // but we still need descriptor sets to point to them.

};

#endif //MY_APPLICATION_RENDERERVULKAN_H