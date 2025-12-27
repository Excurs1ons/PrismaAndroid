#ifndef PRISMA_ANDROID_RENDER_PASS_H
#define PRISMA_ANDROID_RENDER_PASS_H

#include <vulkan/vulkan.h>
#include <string>
#include <utility>
/**
 * 逻辑 RenderPass 基类
 *
 * 设计目的：
 * - 封装一种渲染类型的所有逻辑（Pipeline 创建、数据持有、渲染命令录制）
 * - 为未来切换 RenderAPI（如 DirectX、Metal）提供抽象层
 * - 注意：这不是 Vulkan 的 VkRenderPass，而是逻辑上的渲染通道概念
 *
 * 等价关系：
 * 逻辑 RenderPass ≈ VkPipeline + 数据 + 命令录制
 *
 * API 切换指南：
 * - initialize() / cleanup()：需要适配不同 API 的资源创建/销毁
 * - record()：需要适配不同 API 的命令录制方式
 * - 成员变量：需要根据 API 调整（VkPipeline → ID3D12PipelineState 等）
 */
class RenderPass {
public:
    std::string name = "Unnamed Pass";
    explicit RenderPass(std::string pass_name){
        name = std::move(pass_name);
    }
    virtual ~RenderPass() = default;

    /**
     * 初始化渲染通道
     * @param device          [Vulkan-Specific] 图形设备句柄
     * @param apiRenderPass   [Vulkan-Specific] API 层的 VkRenderPass 对象
     *
     * API 切换时需要修改参数类型：
     * - Vulkan: VkDevice, VkRenderPass
     * - DirectX 12: ID3D12Device（无需 renderPass 参数）
     * - Metal: MTLDevice, MTLRenderPassDescriptor
     */
    virtual void initialize(VkDevice device, VkRenderPass apiRenderPass) = 0;

    /**
     * 记录渲染命令
     * @param cmdBuffer       [Vulkan-Specific] 命令缓冲区
     *
     * API 切换时需要修改参数类型：
     * - Vulkan: VkCommandBuffer
     * - DirectX 12: ID3D12GraphicsCommandList
     * - Metal: MTLRenderCommandEncoder
     */
    virtual void record(VkCommandBuffer cmdBuffer) = 0;

    /**
     * 清理资源
     * @param device          [Vulkan-Specific] 图形设备句柄
     */
    virtual void cleanup(VkDevice device) = 0;

    // 获取 pipeline（用于调试/验证）
    [[nodiscard]] VkPipeline getPipeline() const { return pipeline_; }

protected:
    // [Vulkan-Specific] 以下成员变量在切换 API 时需要替换

    VkPipeline pipeline_ = VK_NULL_HANDLE;           // Vulkan 渲染管线对象
    // DirectX 12: ID3D12PipelineState
    // Metal: MTLRenderPipelineState

    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE; // Vulkan Pipeline 布局
    // DirectX 12: 无需对应（Root Signature 在其他地方管理）
    // Metal: MTLArgumentEncoder

};

#endif //PRISMA_ANDROID_RENDER_PASS_H
