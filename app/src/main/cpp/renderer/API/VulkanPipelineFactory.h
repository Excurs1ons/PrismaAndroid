#ifndef PRISMA_ANDROID_VULKAN_PIPELINE_FACTORY_H
#define PRISMA_ANDROID_VULKAN_PIPELINE_FACTORY_H

#include "PipelineConfig.h"

#if RENDER_API_VULKAN

#include <vulkan/vulkan.h>
#include <vector>

/**
 * @brief Vulkan Graphics Pipeline 实现
 */
class VulkanGraphicsPipeline : public GraphicsPipeline {
public:
    VulkanGraphicsPipeline(VkPipeline pipeline, VkPipelineLayout layout)
        : pipeline_(pipeline), layout_(layout) {}

    ~VulkanGraphicsPipeline() override = default;

    NativePipeline getNative() const override {
        return static_cast<NativePipeline>(pipeline_);
    }

    NativePipelineLayout getLayout() const override {
        return static_cast<NativePipelineLayout>(layout_);
    }

    VkPipeline getVkPipeline() const { return pipeline_; }
    VkPipelineLayout getVkLayout() const { return layout_; }

private:
    VkPipeline pipeline_;
    VkPipelineLayout layout_;
};

/**
 * @brief Vulkan Pipeline 工厂实现
 *
 * 包含所有 Vulkan API 调用，负责创建 Graphics Pipeline
 */
class VulkanPipelineFactory : public PipelineFactory {
public:
    explicit VulkanPipelineFactory(android_app* app) : app_(app) {}

    GraphicsPipeline* createGraphicsPipeline(
        const GraphicsPipelineConfig& config,
        NativeDevice device,
        NativeRenderPass renderPass,
        void* shaderData) override;

    void destroyPipeline(GraphicsPipeline* pipeline, NativeDevice device) override;

private:
    android_app* app_;

    // 辅助方法：创建着色器模块
    VkShaderModule createShaderModule(VkDevice device, const std::vector<uint32_t>& code);

    // 辅助方法：加载着色器
    std::vector<uint32_t> loadShader(const std::string& path);
};

#endif // RENDER_API_VULKAN

#endif //PRISMA_ANDROID_VULKAN_PIPELINE_FACTORY_H
