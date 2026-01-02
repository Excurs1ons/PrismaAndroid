/**
 * @file PostProcessFeature.h
 * @brief 后处理特效（色调映射、颜色分级等）
 */

#pragma once

#include "../IRenderFeature.h"
#include "../../RenderPass.h"

/**
 * @brief 色调映射模式
 */
enum class ToneMappingMode {
    None,
    Linear,
    Reinhard,
    ACES
};

/**
 * @brief 后处理Feature
 *
 * 处理色调映射和颜色分级
 */
class PostProcessFeature : public IRenderFeature {
public:
    PostProcessFeature();
    ~PostProcessFeature() override;

    bool Initialize(IRenderContext& context) override;
    void Cleanup() override;
    void AddRenderPasses(BasicRenderer& renderer) override;
    void Execute(IRenderContext& context, const RenderingData& renderingData) override;

    // 配置
    void SetToneMappingMode(ToneMappingMode mode) { toneMapping_ = mode; }
    void SetExposure(float exposure) { exposure_ = exposure; }
    void SetGamma(float gamma) { gamma_ = gamma; }

private:
    ToneMappingMode toneMapping_ = ToneMappingMode::ACES;
    float exposure_ = 1.0f;
    float gamma_ = 2.2f;

    class ToneMappingPass* toneMappingPass_ = nullptr;
};
