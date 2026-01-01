/**
 * @file DepthPrePass.h
 * @brief 深度预通过通道
 *
 * 功能:
 * - 预先渲染深度缓冲区
 * - 优化后续渲染的Early-Z剔除
 * - 为屏幕空间效果提供深度信息
 */

#pragma once

#include "../../RenderPass.h"
#include "../RenderQueue.h"
#include <memory>

/**
 * @brief 深度预通过Pass
 *
 * 只渲染深度信息，不计算光照
 * 用于优化复杂场景的性能
 */
class DepthPrePass : public RenderPass {
public:
    DepthPrePass();
    ~DepthPrePass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setRenderQueue(RenderQueue* queue) { renderQueue_ = queue; }

    /** 获取渲染的深度纹理 */
    void* getDepthTexture() const { return depthTexture_; }

private:
    RenderQueue* renderQueue_ = nullptr;
    void* depthTexture_ = nullptr;

    void createPipeline(VkDevice device, VkRenderPass renderPass);
};

/**
 * @brief 深度拷贝Pass
 *
 * 将深度缓冲区从一个纹理拷贝到另一个
 */
class CopyDepthPass : public RenderPass {
public:
    CopyDepthPass();
    ~CopyDepthPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setSourceDepth(void* depth) { sourceDepth_ = depth; }
    void setDestDepth(void* depth) { destDepth_ = depth; }

private:
    void* sourceDepth_ = nullptr;
    void* destDepth_ = nullptr;
};

/**
 * @brief 深度可视化Pass
 *
 * 将深度缓冲区可视化输出（调试用）
 */
class DepthVisualizationPass : public RenderPass {
public:
    DepthVisualizationPass();
    ~DepthVisualizationPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setDepthTexture(void* depth) { depthTexture_ = depth; }
    void setOutputTexture(void* output) { outputTexture_ = output; }

    enum class Mode {
        Linear,     // 线性深度
        NonLinear,  // 非线性深度（原始值）
        Heatmap,    // 热力图
        Grayscale   // 灰度
    };
    void setMode(Mode mode) { mode_ = mode; }

private:
    void* depthTexture_ = nullptr;
    void* outputTexture_ = nullptr;
    Mode mode_ = Mode::Linear;
};
