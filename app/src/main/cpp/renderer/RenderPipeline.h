#ifndef PRISMA_ANDROID_RENDER_PIPELINE_H
#define PRISMA_ANDROID_RENDER_PIPELINE_H

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include "OpaquePass.h"
#include "BackgroundPass.h"


// 前向声明
class RenderPass;

/**
 * 逻辑 RenderPipeline
 *
 * 设计目的：
 * - 作为多个 RenderPass 的容器和管理器
 * - 定义渲染顺序
 * - 为未来切换 RenderAPI 提供统一接口
 *
 * 与 Vulkan VkPipeline 的区别：
 * - Vulkan VkPipeline: 描述渲染状态（着色器、混合、深度测试等）
 * - 本类 RenderPipeline: 管理多个渲染通道的执行流程
 *
 * 等价关系：
 * RenderPipeline::execute() 等价于：
 *   vkCmdBeginRenderPass()
 *   + 执行多个 VkPipeline 的渲染命令
 *   + vkCmdEndRenderPass()
 *
 * API 切换指南：
 * - 每个 API 都有类似的"渲染管线"概念，但实现方式不同
 * - Vulkan: 自定义管理，手动提交命令
 * - DirectX 12: 类似 Vulkan，需要手动管理命令列表
 * - Metal: MTLRenderPipelineState + 渲染命令编码器
 */
class RenderPipeline {
public:
    RenderPipeline() = default;
    ~RenderPipeline() = default;

    // 禁止拷贝
    RenderPipeline(const RenderPipeline&) = delete;
    RenderPipeline& operator=(const RenderPipeline&) = delete;

    // 允许移动
    RenderPipeline(RenderPipeline&&) = default;
    RenderPipeline& operator=(RenderPipeline&&) = default;

    /**
     * 添加渲染通道到管线
     * @param pass 渲染通道（转移所有权）
     *
     * 执行顺序：按照添加的顺序执行
     * 例如：先 add BackgroundPass，再 add OpaquePass
     *       则先渲染背景，再渲染不透明物体
     */
    void addPass(std::unique_ptr<RenderPass> pass);

    /**
     * 初始化管线中的所有 Pass
     * @param device        [Vulkan-Specific] 图形设备
     * @param apiRenderPass [Vulkan-Specific] API 层的 VkRenderPass
     *
     * API 切换时需要修改参数类型：
     * - DirectX 12: ID3D12Device
     * - Metal: MTLDevice
     */
    void initialize(VkDevice device, VkRenderPass apiRenderPass);

    /**
     * 执行管线（按顺序执行所有 Pass）
     * @param cmdBuffer [Vulkan-Specific] 命令缓冲区
     *
     * API 切换时需要修改参数类型：
     * - DirectX 12: ID3D12GraphicsCommandList
     * - Metal: MTLRenderCommandEncoder
     */
    void execute(VkCommandBuffer cmdBuffer);

    /**
     * 设置当前帧索引（传递给所有 Pass）
     */
    void setCurrentFrame(uint32_t currentFrame);

    /**
     * 获取 OpaquePass 指针（用于更新 uniform buffer）
     * @return OpaquePass 指针，如果不存在则返回 nullptr
     */
    OpaquePass* getOpaquePass();

    /**
     * 获取 BackgroundPass 指针（用于更新 uniform buffer）
     * @return BackgroundPass 指针，如果不存在则返回 nullptr
     */
    BackgroundPass* getBackgroundPass();

    /**
     * 清理管线中的所有 Pass
     * @param device [Vulkan-Specific] 图形设备
     */
    void cleanup(VkDevice device);

private:
    std::vector<std::unique_ptr<RenderPass>> passes_;  // 渲染通道列表（按添加顺序执行）

    // [Vulkan-Specific] 以下成员变量在切换 API 时需要修改或移除
    VkDevice device_ = VK_NULL_HANDLE;
    VkRenderPass apiRenderPass_ = VK_NULL_HANDLE;
};

#endif //PRISMA_ANDROID_RENDER_PIPELINE_H
