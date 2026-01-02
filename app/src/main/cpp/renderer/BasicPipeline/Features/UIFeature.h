/**
 * @file UIFeature.h
 * @brief UI渲染特效
 */

#pragma once

#include "../IRenderFeature.h"
#include "../../RenderPass.h"

/**
 * @brief UI渲染Feature
 */
class UIFeature : public IRenderFeature {
public:
    UIFeature();
    ~UIFeature() override;

    bool Initialize(IRenderContext& context) override;
    void Cleanup() override;
    void AddRenderPasses(BasicRenderer& renderer) override;
    void Execute(IRenderContext& context, const RenderingData& renderingData) override;

private:
    class UIPass* uiPass_ = nullptr;
    class TextPass* textPass_ = nullptr;
};
