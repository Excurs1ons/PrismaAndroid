/**
 * @file UIPass.h
 * @brief UI覆盖渲染通道
 *
 * 功能:
 * - 渲染UI元素（最后渲染，禁用深度写入）
 * - 文本渲染
 * - 精灵渲染
 */

#pragma once

#include "../../RenderPass.h"
#include "../RenderQueue.h"
#include <memory>

/**
 * @brief UI渲染Pass
 *
 * 在所有3D内容之后渲染UI
 * 特点: 禁用深度写入，深度测试可配置
 */
class UIPass : public RenderPass {
public:
    UIPass();
    ~UIPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setRenderQueue(RenderQueue* queue) { renderQueue_ = queue; }

    /** UI深度测试模式 */
    enum class UIDepthMode {
        /** 不进行深度测试（UI总是在最前） */
        IgnoreDepth,

        /** 启用深度测试（UI可以与3D场景交互） */
        TestDepth
    };
    void setDepthMode(UIDepthMode mode) { depthMode_ = mode; }

private:
    RenderQueue* renderQueue_ = nullptr;
    UIDepthMode depthMode_ = UIDepthMode::IgnoreDepth;
};

/**
 * @brief 文本渲染Pass
 */
class TextPass : public RenderPass {
public:
    TextPass();
    ~TextPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    /** 文本数据 */
    struct TextData {
        float2 position;    // 屏幕坐标(像素)
        float3 color;
        float size;
        const char* text;
    };
    void setTexts(const std::vector<TextData>& texts) { texts_ = texts; }

    void setFontAtlas(void* atlas) { fontAtlas_ = atlas; }

private:
    std::vector<TextData> texts_;
    void* fontAtlas_ = nullptr;
};

/**
 * @brief 精灵渲染Pass
 *
 * 渲染2D精灵
 */
class SpritePass : public RenderPass {
public:
    SpritePass();
    ~SpritePass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    /** 精灵数据 */
    struct SpriteData {
        float2 position;    // 中心位置
        float2 size;        // 尺寸
        float4 color;       // 颜色(RGBA)
        float2 uvMin;       // UV最小值
        float2 uvMax;       // UV最大值
        float rotation;     // 旋转角度(弧度)
        void* texture;      // 纹理
    };
    void setSprites(const std::vector<SpriteData>& sprites) { sprites_ = sprites; }

private:
    std::vector<SpriteData> sprites_;
};

/**
 * @�� 进条渲染Pass
 */
class ProgressBarPass : public RenderPass {
public:
    ProgressBarPass();
    ~ProgressBarPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    struct ProgressBarData {
        float2 position;
        float2 size;
        float progress;     // 0.0 - 1.0
        float3 foregroundColor;
        float3 backgroundColor;
        float cornerRadius;
    };
    void setProgressBars(const std::vector<ProgressBarData>& bars) { bars_ = bars; }

private:
    std::vector<ProgressBarData> bars_;
};
