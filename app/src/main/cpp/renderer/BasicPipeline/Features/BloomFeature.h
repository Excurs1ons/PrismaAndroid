/**
 * @file BloomFeature.h
 * @brief Bloom泛光特效
 */

#pragma once

#include "../IRenderFeature.h"
#include "../../RenderPass.h"

/**
 * @brief Bloom泛光Feature
 */
class BloomFeature : public IRenderFeature {
public:
    BloomFeature();
    ~BloomFeature() override;

    bool Initialize(IRenderContext& context) override;
    void Cleanup() override;
    void AddRenderPasses(BasicRenderer& renderer) override;
    void Execute(IRenderContext& context, const RenderingData& renderingData) override;

    // 配置
    void SetThreshold(float threshold) { threshold_ = threshold; }
    void SetIntensity(float intensity) { intensity_ = intensity; }
    void SetIterations(int iterations) { iterations_ = iterations; }

private:
    // Bloom参数
    float threshold_ = 1.0f;
    float intensity_ = 0.5f;
    int iterations_ = 4;

    // 内部Pass
    class BloomExtractPass* extractPass_ = nullptr;
    class BloomBlurPass* blurPass_ = nullptr;
    class BloomCombinePass* combinePass_ = nullptr;

    // 临时纹理
    void* tempTexture_ = nullptr;
};
