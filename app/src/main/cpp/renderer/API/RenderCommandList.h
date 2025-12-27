#ifndef PRISMA_ANDROID_RENDER_COMMAND_LIST_H
#define PRISMA_ANDROID_RENDER_COMMAND_LIST_H

#include "RenderConfig.h"
#include <cstdint>

/**
 * @file RenderCommandList.h
 * @brief 渲染命令列表抽象接口
 *
 * 这是一个平台无关的命令录制接口
 * 用于将渲染命令录制与具体的图形 API 解耦
 *
 * API 切换时：
 * - Vulkan: 实现 VulkanCommandList，内部使用 VkCommandBuffer
 * - DirectX 12: 实现 D3D12CommandList，内部使用 ID3D12GraphicsCommandList
 * - Metal: 实现 MetalCommandList，内部使用 MTLRenderCommandEncoder
 */
class RenderCommandList {
public:
    virtual ~RenderCommandList() = default;

    /**
     * 设置视口
     */
    virtual void setViewport(float x, float y, float width, float height) = 0;

    /**
     * 设置裁剪矩形
     */
    virtual void setScissor(uint32_t offsetX, uint32_t offsetY,
                           uint32_t width, uint32_t height) = 0;

    /**
     * 绑定图形管线
     */
    virtual void bindGraphicsPipeline(NativePipeline pipeline) = 0;

    /**
     * 绑定顶点缓冲区
     * @param firstBinding 起始绑定槽
     * @param buffers 缓冲区数组
     * @param offsets 偏移量数组
     * @param bindingCount 绑定数量
     */
    virtual void bindVertexBuffers(uint32_t firstBinding,
                                  const NativeBuffer* buffers,
                                  const uint64_t* offsets,
                                  uint32_t bindingCount) = 0;

    /**
     * 绑定索引缓冲区
     * @param buffer 索引缓冲区
     * @param offset 偏移量
     * @param indexType 索引类型 (0=uint16, 1=uint32)
     */
    virtual void bindIndexBuffer(NativeBuffer buffer, uint64_t offset, uint32_t indexType) = 0;

    /**
     * 绑定描述符集
     * @param layout 管线布局
     * @param descriptorSets 描述符集数组
     * @param setCount 描述符集数量
     * @param dynamicOffsetCount 动态偏移数量
     * @param dynamicOffsets 动态偏移数组
     */
    virtual void bindDescriptorSets(NativePipelineLayout layout,
                                   const RenderDescriptorLayout* descriptorSets,
                                   uint32_t setCount,
                                   uint32_t dynamicOffsetCount,
                                   const uint32_t* dynamicOffsets) = 0;

    /**
     * 绘制索引
     */
    virtual void drawIndexed(uint32_t indexCount, uint32_t instanceCount,
                            uint32_t firstIndex, int32_t vertexOffset,
                            uint32_t firstInstance) = 0;

    /**
     * 绘制
     */
    virtual void draw(uint32_t vertexCount, uint32_t instanceCount,
                     uint32_t firstVertex, uint32_t firstInstance) = 0;

    // 索引类型常量
    static constexpr uint32_t INDEX_TYPE_UINT16 = 0;
    static constexpr uint32_t INDEX_TYPE_UINT32 = 1;
};

#endif //PRISMA_ANDROID_RENDER_COMMAND_LIST_H
