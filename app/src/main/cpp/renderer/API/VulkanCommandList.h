#ifndef PRISMA_ANDROID_VULKAN_COMMAND_LIST_H
#define PRISMA_ANDROID_VULKAN_COMMAND_LIST_H

#include "RenderCommandList.h"

#if RENDER_API_VULKAN

#include <vulkan/vulkan.h>

/**
 * @file VulkanCommandList.h
 * @brief Vulkan 命令列表实现
 *
 * 将 RenderCommandList 抽象接口映射到 Vulkan API
 */
class VulkanCommandList : public RenderCommandList {
public:
    explicit VulkanCommandList(VkCommandBuffer cmdBuffer) : cmdBuffer_(cmdBuffer) {}

    void setViewport(float x, float y, float width, float height) override {
        VkViewport viewport{};
        viewport.x = x;
        viewport.y = y;
        viewport.width = width;
        viewport.height = height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmdBuffer_, 0, 1, &viewport);
    }

    void setScissor(uint32_t offsetX, uint32_t offsetY,
                   uint32_t width, uint32_t height) override {
        VkRect2D scissor{};
        scissor.offset = {static_cast<int32_t>(offsetX), static_cast<int32_t>(offsetY)};
        scissor.extent = {width, height};
        vkCmdSetScissor(cmdBuffer_, 0, 1, &scissor);
    }

    void bindGraphicsPipeline(NativePipeline pipeline) override {
        vkCmdBindPipeline(cmdBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                         static_cast<VkPipeline>(pipeline));
    }

    void bindVertexBuffers(uint32_t firstBinding,
                          const NativeBuffer* buffers,
                          const uint64_t* offsets,
                          uint32_t bindingCount) override {
        vkCmdBindVertexBuffers(cmdBuffer_, firstBinding, bindingCount,
                              reinterpret_cast<const VkBuffer*>(buffers), offsets);
    }

    void bindIndexBuffer(NativeBuffer buffer, uint64_t offset, uint32_t indexType) override {
        VkIndexType vkIndexType = (indexType == INDEX_TYPE_UINT32)
            ? VK_INDEX_TYPE_UINT32
            : VK_INDEX_TYPE_UINT16;
        vkCmdBindIndexBuffer(cmdBuffer_, static_cast<VkBuffer>(buffer), offset, vkIndexType);
    }

    void bindDescriptorSets(NativePipelineLayout layout,
                           const RenderDescriptorLayout* descriptorSets,
                           uint32_t setCount,
                           uint32_t dynamicOffsetCount,
                           const uint32_t* dynamicOffsets) override {
        vkCmdBindDescriptorSets(cmdBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                               static_cast<VkPipelineLayout>(layout),
                               0, setCount,
                               reinterpret_cast<const VkDescriptorSet*>(descriptorSets),
                               dynamicOffsetCount, dynamicOffsets);
    }

    void drawIndexed(uint32_t indexCount, uint32_t instanceCount,
                    uint32_t firstIndex, int32_t vertexOffset,
                    uint32_t firstInstance) override {
        vkCmdDrawIndexed(cmdBuffer_, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void draw(uint32_t vertexCount, uint32_t instanceCount,
             uint32_t firstVertex, uint32_t firstInstance) override {
        vkCmdDraw(cmdBuffer_, vertexCount, instanceCount, firstVertex, firstInstance);
    }

private:
    VkCommandBuffer cmdBuffer_;
};

#endif // RENDER_API_VULKAN

#endif //PRISMA_ANDROID_VULKAN_COMMAND_LIST_H
