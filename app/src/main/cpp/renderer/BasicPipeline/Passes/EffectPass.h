/**
 * @file EffectPass.h
 * @brief 特效渲染通道
 *
 * 包含:
 * - 轮廓渲染 (Outline)
 * - 贴花 (Decal)
 * - 全屏效果
 * - 转场效果
 */

#pragma once

#include "../../RenderPass.h"
#include "../RenderQueue.h"
#include <memory>

// ============================================================================
// Outline Pass (轮廓渲染)
// ============================================================================

/**
 * @brief 轮廓渲染方法
 */
enum class OutlineMethod {
    /** 后处理方法（基于颜色或深度） */
    PostProcess,

    /** 几何方法（放大网格） */
    Geometric,

    /** 模板缓冲区方法 */
    Stencil
};

/**
 * @brief 轮廓渲染Pass
 *
 * 为选中物体渲染轮廓
 */
class OutlinePass : public RenderPass {
public:
    OutlinePass();
    ~OutlinePass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setRenderQueue(RenderQueue* queue) { renderQueue_ = queue; }

    void setOutlineColor(const float3& color) { outlineColor_ = color; }
    void setOutlineWidth(float width) { outlineWidth_ = width; }
    void setOutlineMethod(OutlineMethod method) { method_ = method; }

    /** 使用模板缓冲区渲染轮廓 */
    void useStencilBuffer(bool use, uint32_t reference = 1);

private:
    RenderQueue* renderQueue_ = nullptr;
    float3 outlineColor_ = {1, 1, 0};
    float outlineWidth_ = 2.0f;
    OutlineMethod method_ = OutlineMethod::Geometric;

    bool useStencil_ = false;
    uint32_t stencilReference_ = 1;
};

// ============================================================================
// Selection Highlight Pass (选中高亮)
// ============================================================================

/**
 * @brief 选中高亮Pass
 *
 * 高亮选中的物体（类似编辑器）
 */
class SelectionHighlightPass : public RenderPass {
public:
    SelectionHighlightPass();
    ~SelectionHighlightPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setSelectedObjects(const std::vector<void*>& objects) { selectedObjects_ = objects; }

    void setHighlightColor(const float3& color) { highlightColor_ = color; }
    void setHighlightIntensity(float intensity) { intensity_ = intensity; }

private:
    std::vector<void*> selectedObjects_;
    float3 highlightColor_ = {1, 1, 0};
    float intensity_ = 0.5f;
};

// ============================================================================
// Decal Pass (贴花)
// ============================================================================

/**
 * @brief 贴花Pass
 *
 * 在物体表面渲染贴花（血迹、弹孔、涂鸦等）
 */
class DecalPass : public RenderPass {
public:
    DecalPass();
    ~DecalPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    /** 贴花数据 */
    struct DecalData {
        float4x4 transform;      // 贴花变换矩阵
        void* albedoTexture;     // 反照率纹理
        void* normalTexture;     // 法线纹理
        void* maskTexture;       // 遮罩纹理
        float3 albedoColor;
        float normalStrength;
        float opacity;
        float fadeDistance;      // 渐隐距离
    };

    void setDecals(const std::vector<DecalData>& decals) { decals_ = decals; }

    void addDecal(const DecalData& decal) { decals_.push_back(decal); }
    void clearDecals() { decals_.clear(); }

private:
    std::vector<DecalData> decals_;
};

// ============================================================================
// Trail Renderer Pass (拖尾渲染)
// ============================================================================

/**
 * @brief 拖尾渲染Pass
 *
 * 渲染物体移动时的拖尾效果
 */
class TrailRendererPass : public RenderPass {
public:
    TrailRendererPass();
    ~TrailRendererPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    /** 拖尾数据 */
    struct TrailData {
        struct Point {
            float3 position;
            float width;
            float4 color;
        };
        std::vector<Point> points;

        void* texture;
        bool smooth;      // 平滑连接
        bool loop;        // 循环
    };

    void setTrails(const std::vector<TrailData>& trails) { trails_ = trails; }

private:
    std::vector<TrailData> trails_;
};

// ============================================================================
// Line Renderer Pass (线条渲染)
// ============================================================================

/**
 * @brief 线条渲染Pass
 *
 * 渲染3D线条（激光、光束等）
 */
class LineRendererPass : public RenderPass {
public:
    LineRendererPass();
    ~LineRendererPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    /** 线条数据 */
    struct LineData {
        float3 start;
        float3 end;
        float3 color;
        float width;
        bool fade;       // 两端渐隐
    };

    void setLines(const std::vector<LineData>& lines) { lines_ = lines; }

    void addLine(const LineData& line) { lines_.push_back(line); }
    void clearLines() { lines_.clear(); }

private:
    std::vector<LineData> lines_;
};

// ============================================================================
// Transition Pass (转场效果)
// ============================================================================

/**
 * @brief 转场效果类型
 */
enum class TransitionType {
    Fade,           // 淡入淡出
    Wipe,           // 擦除
    Dissolve,       // 溶解
    Circle,         // 圆形展开
    Zoom,           // 缩放
    Slide,          // 滑动
    Custom          // 自定义纹理
};

/**
 * @brief 转场Pass
 *
 * 场景之间的转场效果
 */
class TransitionPass : public RenderPass {
public:
    TransitionPass();
    ~TransitionPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setFromTexture(void* texture) { fromTexture_ = texture; }
    void setToTexture(void* texture) { toTexture_ = texture; }
    void setOutputTexture(void* texture) { outputTexture_ = texture; }

    void setTransitionType(TransitionType type) { type_ = type; }
    void setProgress(float progress) { progress_ = progress; }  // 0-1
    void setTransitionTexture(void* texture) { transitionTexture_ = texture; }

    /** 检查转场是否完成 */
    bool isComplete() const { return progress_ >= 1.0f; }

private:
    void* fromTexture_ = nullptr;
    void* toTexture_ = nullptr;
    void* outputTexture_ = nullptr;
    void* transitionTexture_ = nullptr;

    TransitionType type_ = TransitionType::Fade;
    float progress_ = 0.0f;
};

// ============================================================================
// Full Screen Effect Pass (全屏效果)
// ============================================================================

/**
 * @brief 全屏效果Pass
 *
 * 全屏颜色调整效果
 */
class FullScreenEffectPass : public RenderPass {
public:
    FullScreenEffectPass();
    ~FullScreenEffectPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setInputTexture(void* texture) { inputTexture_ = texture; }
    void setOutputTexture(void* texture) { outputTexture_ = texture; }

    /** 颜色调整 */
    void setBrightness(float brightness) { brightness_ = brightness; }
    void setContrast(float contrast) { contrast_ = contrast; }
    void setSaturation(float saturation) { saturation_ = saturation; }
    void setHue(float hue) { hue_ = hue; }

    /** 颜色叠加 */
    void setColorOverlay(const float3& color) { colorOverlay_ = color; }
    void setColorOverlayIntensity(float intensity) { colorOverlayIntensity_ = intensity; }

    /** 晕影 */
    void setVignetteIntensity(float intensity) { vignetteIntensity_ = intensity; }
    void setVignetteColor(const float3& color) { vignetteColor_ = color; }

    /** 色差 */
    void setChromaticAberration(float intensity) { chromaticAberration_ = intensity; }

    /** 扫描线 */
    void setScanlinesIntensity(float intensity) { scanlinesIntensity_ = intensity; }
    void setScanlinesCount(int count) { scanlinesCount_ = count; }

    /** 噪点 */
    void setNoiseIntensity(float intensity) { noiseIntensity_ = intensity; }

    /** 像素化 */
    void setPixelSize(float size) { pixelSize_ = size; }

    /** 重置所有效果 */
    void reset() {
        brightness_ = 0.0f;
        contrast_ = 1.0f;
        saturation_ = 1.0f;
        hue_ = 0.0f;
        colorOverlay_ = {0, 0, 0};
        colorOverlayIntensity_ = 0.0f;
        vignetteIntensity_ = 0.0f;
        vignetteColor_ = {0, 0, 0};
        chromaticAberration_ = 0.0f;
        scanlinesIntensity_ = 0.0f;
        scanlinesCount_ = 0;
        noiseIntensity_ = 0.0f;
        pixelSize_ = 1.0f;
    }

private:
    void* inputTexture_ = nullptr;
    void* outputTexture_ = nullptr;

    // 颜色调整
    float brightness_ = 0.0f;
    float contrast_ = 1.0f;
    float saturation_ = 1.0f;
    float hue_ = 0.0f;

    // 颜色叠加
    float3 colorOverlay_ = {0, 0, 0};
    float colorOverlayIntensity_ = 0.0f;

    // 晕影
    float vignetteIntensity_ = 0.0f;
    float3 vignetteColor_ = {0, 0, 0};

    // 色差
    float chromaticAberration_ = 0.0f;

    // 扫描线
    float scanlinesIntensity_ = 0.0f;
    int scanlinesCount_ = 0;

    // 噪点
    float noiseIntensity_ = 0.0f;

    // 像素化
    float pixelSize_ = 1.0f;
};
