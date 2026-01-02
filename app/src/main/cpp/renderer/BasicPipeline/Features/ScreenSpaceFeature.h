/**
 * @file ScreenSpaceFeature.h
 * @brief 屏幕空间特效（SSAO、SSR等）
 */

#pragma once

#include "../IRenderFeature.h"
#include "../../RenderPass.h"

/**
 * @brief SSAO Feature
 */
class SSAOFeature : public IRenderFeature {
public:
    SSAOFeature();
    ~SSAOFeature() override;

    bool Initialize(IRenderContext& context) override;
    void Cleanup() override;
    void AddRenderPasses(BasicRenderer& renderer) override;
    void Execute(IRenderContext& context, const RenderingData& renderingData) override;

    void SetSampleCount(int count) { sampleCount_ = count; }
    void SetRadius(float radius) { radius_ = radius; }
    void SetIntensity(float intensity) { intensity_ = intensity; }

private:
    int sampleCount_ = 64;
    float radius_ = 0.5f;
    float intensity_ = 1.0f;

    class SSAOPass* ssaoPass_ = nullptr;
    class SSAOBlurPass* blurPass_ = nullptr;
};

/**
 * @brief SSR Feature
 */
class SSRFeature : public IRenderFeature {
public:
    SSRFeature();
    ~SSRFeature() override;

    bool Initialize(IRenderContext& context) override;
    void Cleanup() override;
    void AddRenderPasses(BasicRenderer& renderer) override;
    void Execute(IRenderContext& context, const RenderingData& renderingData) override;

    void SetMaxIterations(int count) { maxIterations_ = count; }
    void SetThickness(float thickness) { thickness_ = thickness; }

private:
    int maxIterations_ = 128;
    float thickness_ = 0.01f;

    class SSRPass* ssrPass_ = nullptr;
};
