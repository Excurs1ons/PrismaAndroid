/**
 * @file DepthPrepassFeature.h
 * @brief 深度预通过特效（性能优化）
 */

#pragma once

#include "../IRenderFeature.h"
#include "../../RenderPass.h"

/**
 * @brief 深度预通过Feature
 *
 * 预先渲染深度，优化后续渲染的Early-Z剔除
 */
class DepthPrepassFeature : public IRenderFeature {
public:
    DepthPrepassFeature();
    ~DepthPrepassFeature() override;

    bool Initialize(IRenderContext& context) override;
    void Cleanup() override;
    void AddRenderPasses(BasicRenderer& renderer) override;
    void Execute(IRenderContext& context, const RenderingData& renderingData) override;

    void* GetDepthTexture() const { return depthTexture_; }

private:
    class DepthPrePass* depthPass_ = nullptr;
    void* depthTexture_ = nullptr;
};
