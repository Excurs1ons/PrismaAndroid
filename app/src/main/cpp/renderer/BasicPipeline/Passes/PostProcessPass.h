/**
 * @file PostProcessPass.h
 * @brief 后处理通道基类和常见后处理效果
 *
 * 后处理流程:
 * 1. Color Grading (颜色分级)
 * 2. Tone Mapping (色调映射)
 * 3. Gamma Correction (伽马校正)
 * 4. FXAA (抗锯齿)
 */

#pragma once

#include "../../RenderPass.h"
#include "../RenderingData.h"
#include <memory>

/**
 * @brief 后处理效果类型
 */
enum class PostProcessEffect {
    None = 0,
    ColorGrading,
    ToneMapping,
    GammaCorrection,
    Bloom,
    MotionBlur,
    DepthOfField,
    AmbientOcclusion,
    ScreenSpaceReflections,
    AntiAliasing
};

/**
 * @brief 色调映射模式
 */
enum class ToneMappingMode {
    None,           // 不映射
    Linear,         // 线性映射
    Reinhard,       // Reinhard映射
    ACES,           // ACES电影级映射
    AgX,            // AgX映射
    Neutral         // Neutral映射
};

/**
 * @brief 后处理设置
 */
struct PostProcessSettings {
    // 色调映射
    ToneMappingMode toneMapping = ToneMappingMode::ACES;
    float exposure = 1.0f;
    float contrast = 1.0f;
    float saturation = 1.0f;

    // 伽马校正
    bool enableGammaCorrection = true;
    float gamma = 2.2f;

    // Bloom
    bool enableBloom = false;
    float bloomThreshold = 1.0f;
    float bloomIntensity = 0.5f;
    int bloomIterations = 4;

    // 景深
    bool enableDOF = false;
    float focusDistance = 10.0f;
    float aperture = 5.6f;
    float focalLength = 50.0f;

    // 运动模糊
    bool enableMotionBlur = false;
    float motionBlurIntensity = 0.5f;
    int motionBlurSamples = 8;
};

/**
 * @brief 颜色分级Pass
 *
 * 调整图像的整体外观
 */
class ColorGradingPass : public RenderPass {
public:
    ColorGradingPass();
    ~ColorGradingPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setInputTexture(void* texture) { inputTexture_ = texture; }
    void setOutputTexture(void* texture) { outputTexture_ = texture; }
    void setSettings(const PostProcessSettings* settings) { settings_ = settings; }

private:
    void* inputTexture_ = nullptr;
    void* outputTexture_ = nullptr;
    const PostProcessSettings* settings_ = nullptr;
};

/**
 * @brief 色调映射Pass
 *
 * 将HDR颜色映射到LDR范围
 */
class ToneMappingPass : public RenderPass {
public:
    ToneMappingPass();
    ~ToneMappingPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setInputTexture(void* texture) { inputTexture_ = texture; }
    void setOutputTexture(void* texture) { outputTexture_ = texture; }
    void setMode(ToneMappingMode mode) { mode_ = mode; }
    void setExposure(float exposure) { exposure_ = exposure; }

private:
    void* inputTexture_ = nullptr;
    void* outputTexture_ = nullptr;
    ToneMappingMode mode_ = ToneMappingMode::ACES;
    float exposure_ = 1.0f;
};

/**
 * @brief Bloom Pass
 *
 * 泛光效果
 */
class BloomPass : public RenderPass {
public:
    BloomPass();
    ~BloomPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setInputTexture(void* texture) { inputTexture_ = texture; }
    void setOutputTexture(void* texture) { outputTexture_ = texture; }
    void setThreshold(float threshold) { threshold_ = threshold; }
    void setIntensity(float intensity) { intensity_ = intensity; }
    void setIterations(int iterations) { iterations_ = iterations; }

private:
    void* inputTexture_ = nullptr;
    void* outputTexture_ = nullptr;
    float threshold_ = 1.0f;
    float intensity_ = 0.5f;
    int iterations_ = 4;

    // 中间纹理
    std::vector<void*> downsampleTextures_;
    std::vector<void*> blurTextures_;
};

/**
 * @brief 综合后处理Pass
 *
 * 一次性应用所有后处理效果
 */
class PostProcessPass : public RenderPass {
public:
    PostProcessPass();
    ~PostProcessPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setInputTexture(void* texture) { inputTexture_ = texture; }
    void setOutputTexture(void* texture) { outputTexture_ = texture; }
    void setSettings(const PostProcessSettings& settings) { settings_ = settings; }

private:
    void* inputTexture_ = nullptr;
    void* outputTexture_ = nullptr;
    PostProcessSettings settings_;
};
