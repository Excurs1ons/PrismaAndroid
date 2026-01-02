/**
 * @file DebugFeature.h
 * @brief 调试可视化特效
 */

#pragma once

#include "../IRenderFeature.h"
#include "../../RenderPass.h"

/**
 * @brief 调试可视化模式
 */
enum class DebugViewMode {
    None,
    Wireframe,
    Normals,
    UV,
    Depth,
    Albedo,
    Metallic,
    Roughness,
    Lighting,
    Shadows
};

/**
 * @brief 调试Feature
 */
class DebugFeature : public IRenderFeature {
public:
    DebugFeature();
    ~DebugFeature() override;

    bool Initialize(IRenderContext& context) override;
    void Cleanup() override;
    void AddRenderPasses(BasicRenderer& renderer) override;
    void Execute(IRenderContext& context, const RenderingData& renderingData) override;

    void SetDebugMode(DebugViewMode mode) { debugMode_ = mode; }
    void SetShowBounds(bool show) { showBounds_ = show; }
    void SetShowLights(bool show) { showLights_ = show; }

private:
    DebugViewMode debugMode_ = DebugViewMode::None;
    bool showBounds_ = false;
    bool showLights_ = false;

    class DebugRenderPass* debugPass_ = nullptr;
    class WireframePass* wireframePass_ = nullptr;
    class BoundsPass* boundsPass_ = nullptr;
};
