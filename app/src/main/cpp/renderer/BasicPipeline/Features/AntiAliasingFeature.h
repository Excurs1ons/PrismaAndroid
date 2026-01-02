/**
 * @file AntiAliasingFeature.h
 * @brief 抗锯齿特效（FXAA、TAA等）
 */

#pragma once

#include "../IRenderFeature.h"
#include "../../RenderPass.h"

/**
 * @brief 抗锯齿模式
 */
enum class AntiAliasingMode {
    None,
    FXAA,
    TAA
};

/**
 * @brief 抗锯齿Feature
 */
class AntiAliasingFeature : public IRenderFeature {
public:
    AntiAliasingFeature();
    ~AntiAliasingFeature() override;

    bool Initialize(IRenderContext& context) override;
    void Cleanup() override;
    void AddRenderPasses(BasicRenderer& renderer) override;
    void Execute(IRenderContext& context, const RenderingData& renderingData) override;

    void SetMode(AntiAliasingMode mode) { mode_ = mode; }

    // TAA参数
    void SetJitterOffset(float2 offset) { jitterOffset_ = offset; }
    void SetFeedbackMin(float min) { feedbackMin_ = min; }
    void SetFeedbackMax(float max) { feedbackMax_ = max; }

private:
    AntiAliasingMode mode_ = AntiAliasingMode::FXAA;

    // TAA
    float2 jitterOffset_ = {0, 0};
    float feedbackMin_ = 0.88f;
    float feedbackMax_ = 0.97f;

    class FxaaPass* fxaaPass_ = nullptr;
    class TaaPass* taaPass_ = nullptr;
};
