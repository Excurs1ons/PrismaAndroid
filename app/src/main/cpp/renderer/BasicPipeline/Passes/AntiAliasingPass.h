/**
 * @file AntiAliasingPass.h
 * @brief 抗锯齿通道
 *
 * 支持的抗锯齿算法:
 * - FXAA (Fast Approximate Anti-Aliasing)
 * - SMAA (Subpixel Morphological Anti-Aliasing)
 * - MSAA (Multisample Anti-Aliasing) - 需要渲染时启用
 * - TAA (Temporal Anti-Aliasing)
 */

#pragma once

#include "../../RenderPass.h"
#include <memory>

/**
 * @brief 抗锯齿模式
 */
enum class AntiAliasingMode {
    None,
    FXAA,
    SMAA,
    TAA,
    MSAA
};

/**
 * @brief FXAA Pass
 *
 * 快速近似抗锯齿
 */
class FxaaPass : public RenderPass {
public:
    FxaaPass();
    ~FxaaPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setInputTexture(void* texture) { inputTexture_ = texture; }
    void setOutputTexture(void* texture) { outputTexture_ = texture; }

    /** FXAA质量预设 */
    enum class QualityPreset {
        Low,        // 低质量（最快）
        Medium,     // 中等质量
        High,       // 高质量
        Ultra       // 超高质量
    };
    void setQualityPreset(QualityPreset preset) { qualityPreset_ = preset; }

    /** 自定义参数 */
    void setEdgeThreshold(float threshold) { edgeThreshold_ = threshold; }
    void setEdgeThresholdMin(float min) { edgeThresholdMin_ = min; }
    void setSearchSteps(int steps) { searchSteps_ = steps; }

private:
    void* inputTexture_ = nullptr;
    void* outputTexture_ = nullptr;
    QualityPreset qualityPreset_ = QualityPreset::High;

    // FXAA参数
    float edgeThreshold_ = 0.125f;
    float edgeThresholdMin_ = 0.0625f;
    int searchSteps_ = 32;
};

/**
 * @brief SMAA Pass
 *
 * 子像素形态学抗锯齿
 */
class SmaaPass : public RenderPass {
public:
    SmaaPass();
    ~SmaaPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setInputTexture(void* texture) { inputTexture_ = texture; }
    void setOutputTexture(void* texture) { outputTexture_ = texture; }

    /** SMAA质量预设 */
    enum class QualityPreset {
        Low,        // SMAA 1x
        Medium,     // SMAA S
        High,       // SMAA T2x
        Ultra       // SMAA 4x
    };
    void setQualityPreset(QualityPreset preset) { qualityPreset_ = preset; }

    /** 边缘检测模式 */
    enum class EdgeDetection {
        Depth,      // 基于深度
        Color,      // 基于颜色
        Luma        // 基于亮度
    };
    void setEdgeDetectionMode(EdgeDetection mode) { edgeDetection_ = mode; }

private:
    void* inputTexture_ = nullptr;
    void* outputTexture_ = nullptr;
    QualityPreset qualityPreset_ = QualityPreset::High;
    EdgeDetection edgeDetection_ = EdgeDetection::Luma;

    // SMAA纹理（边缘检测、混合权重）
    void* edgesTexture_ = nullptr;
    void* blendTexture_ = nullptr;
    void* areaTexture_ = nullptr;   // 预计算的区域纹理
    void* searchTexture_ = nullptr; // 预计算的搜索纹理
};

/**
 * @brief TAA Pass
 *
 * 时态抗锯齿
 * 需要历史帧数据
 */
class TaaPass : public RenderPass {
public:
    TaaPass();
    ~TaaPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setCurrentTexture(void* texture) { currentTexture_ = texture; }
    void setPreviousTexture(void* texture) { previousTexture_ = texture; }
    void setVelocityTexture(void* texture) { velocityTexture_ = texture; }
    void setDepthTexture(void* texture) { depthTexture_ = texture; }
    void setOutputTexture(void* texture) { outputTexture_ = texture; }

    /** 设置相机抖动（需要在渲染时应用） */
    void setJitterOffset(float2 offset) { jitterOffset_ = offset; }

    /** TAA参数 */
    void setFeedbackMin(float min) { feedbackMin_ = min; }
    void setFeedbackMax(float max) { feedbackMax_ = max; }
    void setMotionBlurStrength(float strength) { motionBlurStrength_ = strength; }

private:
    void* currentTexture_ = nullptr;
    void* previousTexture_ = nullptr;
    void* velocityTexture_ = nullptr;
    void* depthTexture_ = nullptr;
    void* outputTexture_ = nullptr;

    float2 jitterOffset_ = {0, 0};
    float feedbackMin_ = 0.88f;
    float feedbackMax_ = 0.97f;
    float motionBlurStrength_ = 0.5f;
};

/**
 * @brief 综合抗锯齿Pass
 *
 * 根据配置选择合适的抗锯齿方法
 */
class AntiAliasingPass : public RenderPass {
public:
    AntiAliasingPass();
    ~AntiAliasingPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setInputTexture(void* texture) { inputTexture_ = texture; }
    void setOutputTexture(void* texture) { outputTexture_ = texture; }
    void setMode(AntiAliasingMode mode) { mode_ = mode; }

private:
    void* inputTexture_ = nullptr;
    void* outputTexture_ = nullptr;
    AntiAliasingMode mode_ = AntiAliasingMode::FXAA;

    // 子Pass
    std::unique_ptr<FxaaPass> fxaaPass_;
    std::unique_ptr<SmaaPass> smaaPass_;
    std::unique_ptr<TaaPass> taaPass_;
};
