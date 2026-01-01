/**
 * @file DebugPass.h
 * @brief 调试可视化通道
 *
 * 调试渲染Pass:
 * - 法线可视化
 * - 切线可视化
 * - UV可视化
 * - 网格线框
 * - 包围盒
 * - 光照方向
 * - 相机视锥体
 */

#pragma once

#include "../../RenderPass.h"
#include "../RenderingData.h"
#include "../RenderQueue.h"
#include <memory>

// ============================================================================
// 调试可视化Pass
// ============================================================================

/**
 * @brief 调试渲染模式
 */
enum class DebugRenderMode {
    None = 0,
    Wireframe,
    Normals,
    Tangents,
    Bitangents,
    UV,
    VertexColors,
    Depth,
    LinearDepth,
    Albedo,
    Metallic,
    Roughness,
    AO,
    Emission,
    Specular,
    Lighting,
    Shadows,
    Reflections,
    Overdraw
};

/**
 * @brief 调试渲染Pass
 *
 * 将场景以调试模式渲染
 */
class DebugRenderPass : public RenderPass {
public:
    DebugRenderPass();
    ~DebugRenderPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setRenderQueue(RenderQueue* queue) { renderQueue_ = queue; }
    void setRenderingData(const RenderingData* data) { renderingData_ = data; }

    void setDebugMode(DebugRenderMode mode) { debugMode_ = mode; }

private:
    RenderQueue* renderQueue_ = nullptr;
    const RenderingData* renderingData_ = nullptr;
    DebugRenderMode debugMode_ = DebugRenderMode::None;
};

// ============================================================================
// 线框Pass
// ============================================================================

/**
 * @brief 线框渲染Pass
 *
 * 渲染网格的线框
 */
class WireframePass : public RenderPass {
public:
    WireframePass();
    ~WireframePass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setRenderQueue(RenderQueue* queue) { renderQueue_ = queue; }
    void setWireframeColor(const float3& color) { wireColor_ = color; }
    void setLineWidth(float width) { lineWidth_ = width; }

private:
    RenderQueue* renderQueue_ = nullptr;
    float3 wireColor_ = {0, 1, 0};
    float lineWidth_ = 1.0f;
};

// ============================================================================
// 包围盒Pass
// ============================================================================

/**
 * @brief 包围盒渲染Pass
 *
 * 渲染物体/网格的包围盒
 */
class BoundsPass : public RenderPass {
public:
    BoundsPass();
    ~BoundsPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setRenderQueue(RenderQueue* queue) { renderQueue_ = queue; }

    /** 包围盒类型 */
    enum class BoundsType {
        None,
        MeshBounds,     // 网格包围盒
        ObjectBounds,   // 物体包围盒（包含变换）
        Both
    };
    void setBoundsType(BoundsType type) { boundsType_ = type; }

    void setBoundsColor(const float3& color) { boundsColor_ = color; }

private:
    RenderQueue* renderQueue_ = nullptr;
    BoundsType boundsType_ = BoundsType::Both;
    float3 boundsColor_ = {1, 0, 1};
};

// ============================================================================
// 光源可视化Pass
// ============================================================================

/**
 * @brief 光源可视化Pass
 *
 * 渲染光源的位置和方向
 */
class LightVisualizationPass : public RenderPass {
public:
    LightVisualizationPass();
    ~LightVisualizationPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    /** 光源数据 */
    struct LightDebugData {
        float3 position;
        float3 direction;
        float3 color;
        float range;
        int type;  // 0=定向光, 1=点光源, 2=聚光灯
    };
    void setLights(const std::vector<LightDebugData>& lights) { lights_ = lights; }

    void setShowDirection(bool show) { showDirection_ = show; }
    void setShowRange(bool show) { showRange_ = show; }

private:
    std::vector<LightDebugData> lights_;
    bool showDirection_ = true;
    bool showRange_ = true;
};

// ============================================================================
// 相机视锥体Pass
// ============================================================================

/**
 * @brief 相机视锥体可视化Pass
 *
 * 渲染相机的视锥体
 */
class CameraFrustumPass : public RenderPass {
public:
    CameraFrustumPass();
    ~CameraFrustumPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    /** 相机数据 */
    struct CameraDebugData {
        float4x4 viewMatrix;
        float4x4 projectionMatrix;
        float4x4 viewProjectionMatrix;
        float3 position;
        float nearPlane;
        float farPlane;
        float fov;
        float aspect;
    };
    void setCamera(const CameraDebugData& camera) { camera_ = camera; }

    void setFrustumColor(const float3& color) { frustumColor_ = color; }

private:
    CameraDebugData camera_;
    float3 frustumColor_ = {1, 1, 0};
};

// ============================================================================
// Overdraw可视化Pass
// ============================================================================

/**
 * @brief Overdraw可视化Pass
 *
 * 可视化过度绘制（性能分析）
 */
class OverdrawVisualizationPass : public RenderPass {
public:
    OverdrawVisualizationPass();
    ~OverdrawVisualizationPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setRenderQueue(RenderQueue* queue) { renderQueue_ = queue; }

    /** Overdraw模式 */
    enum class OverdrawMode {
        Count,      // 计数模式
        Heatmap,    // 热力图模式
        Gradient    // 渐变模式
    };
    void setMode(OverdrawMode mode) { mode_ = mode; }

private:
    RenderQueue* renderQueue_ = nullptr;
    OverdrawMode mode_ = OverdrawMode::Heatmap;
};

// ============================================================================
// 调试文本Pass
// ============================================================================

/**
 * @brief 调试文本Pass
 *
 * 在屏幕上显示调试信息
 */
class DebugTextPass : public RenderPass {
public:
    DebugTextPass();
    ~DebugTextPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    /** 添加调试文本 */
    void addText(const char* text, float2 position, float3 color = {1, 1, 1});
    void addLine(const char* text, float y, float3 color = {1, 1, 1});

    void clear() { debugLines_.clear(); }

    void setFont(void* font) { font_ = font; }

private:
    struct DebugLine {
        const char* text;
        float2 position;
        float3 color;
    };
    std::vector<DebugLine> debugLines_;

    void* font_ = nullptr;
    float currentY_ = 10.0f;
};
