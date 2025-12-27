#ifndef PRISMA_ANDROID_VULKAN_RENDER_PASS_H
#define PRISMA_ANDROID_VULKAN_RENDER_PASS_H

#include "RenderConfig.h"
#include "RenderCommandList.h"
#include <string>
#include <vector>

#if RENDER_API_VULKAN

/**
 * @brief Vulkan 特定的渲染通道基类
 *
 * 这个类包含 Vulkan API 相关的方法和成员
 * 切换到其他 API 时，使用对应的类（如 D3D12RenderPass）
 */
class VulkanRenderPass {
public:
    explicit VulkanRenderPass(std::string name) : name_(std::move(name)) {}
    virtual ~VulkanRenderPass() = default;

    /**
     * 初始化渲染通道（创建 Pipeline）
     * @param device 渲染设备（API 特定）
     * @param renderPass 渲染通道（API 特定）
     */
    virtual void initialize(NativeDevice device, NativeRenderPass renderPass) = 0;

    /**
     * 记录渲染命令（使用命令列表抽象）
     * @param cmdList 命令列表抽象
     */
    virtual void record(RenderCommandList* cmdList) = 0;

    /**
     * 清理资源
     * @param device 渲染设备（API 特定）
     */
    virtual void cleanup(NativeDevice device) = 0;

    /**
     * 获取 Pipeline（API 特定）
     */
    NativePipeline getPipeline() const { return pipeline_; }

    /**
     * 获取 Pipeline Layout（API 特定）
     */
    NativePipelineLayout getPipelineLayout() const { return pipelineLayout_; }

    /**
     * 获取 Pass 名称
     */
    const std::string& getName() const { return name_; }

protected:
    std::string name_;                              // Pass 名称（用于调试）
    NativePipeline pipeline_ = nullptr;             // 渲染管线对象（API 特定）
    NativePipelineLayout pipelineLayout_ = nullptr; // Pipeline 布局（API 特定）
};

#else

#error "VulkanRenderPass is only available when RENDER_API_VULKAN is defined"

#endif // RENDER_API_VULKAN

#endif //PRISMA_ANDROID_VULKAN_RENDER_PASS_H
