/**
 * @file ScreenSpacePass.h
 * @brief 屏幕空间效果通道
 *
 * 包含:
 * - SSAO (Screen Space Ambient Occlusion)
 * - SSR (Screen Space Reflections)
 * - SSGI (Screen Space Global Illumination)
 * - Motion Blur
 */

#pragma once

#include "../../RenderPass.h"
#include "../RenderingData.h"
#include <memory>

// ============================================================================
// SSAO (Screen Space Ambient Occlusion)
// ============================================================================

/**
 * @brief SSAO Pass
 *
 * 屏幕空间环境光遮蔽
 */
class SSAOPass : public RenderPass {
public:
    SSAOPass();
    ~SSAOPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setDepthTexture(void* depth) { depthTexture_ = depth; }
    void setNormalTexture(void* normal) { normalTexture_ = normal; }
    void setOutputTexture(void* output) { outputTexture_ = output; }

    /** SSAO参数 */
    void setSampleCount(int count) { sampleCount_ = count; }
    void setRadius(float radius) { radius_ = radius; }
    void setBias(float bias) { bias_ = bias; }
    void setIntensity(float intensity) { intensity_ = intensity; }

    /** 获取AO纹理（用于光照Pass） */
    void* getAOTexture() const { return aoTexture_; }

private:
    void* depthTexture_ = nullptr;
    void* normalTexture_ = nullptr;
    void* outputTexture_ = nullptr;
    void* aoTexture_ = nullptr;

    // SSAO参数
    int sampleCount_ = 64;
    float radius_ = 0.5f;
    float bias_ = 0.025f;
    float intensity_ = 1.0f;

    // 采样核纹理
    void* sampleKernelTexture_ = nullptr;
    // 噪声纹理
    void* noiseTexture_ = nullptr;

    void generateSampleKernel();
    void generateNoiseTexture();
};

/**
 * @brief SSAO模糊Pass
 *
 * 对AO结果进行模糊，消除噪点
 */
class SSAOBlurPass : public RenderPass {
public:
    SSAOBlurPass();
    ~SSAOBlurPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setInputAO(void* ao) { inputAO_ = ao; }
    void setOutputAO(void* output) { outputAO_ = output; }
    void setBlurRadius(int radius) { blurRadius_ = radius; }

private:
    void* inputAO_ = nullptr;
    void* outputAO_ = nullptr;
    int blurRadius_ = 4;
};

// ============================================================================
// SSR (Screen Space Reflections)
// ============================================================================

/**
 * @brief SSR Pass
 *
 * 屏幕空间反射
 */
class SSRPass : public RenderPass {
public:
    SSRPass();
    ~SSRPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setColorTexture(void* color) { colorTexture_ = color; }
    void setDepthTexture(void* depth) { depthTexture_ = depth; }
    void setNormalTexture(void* normal) { normalTexture_ = normal; }
    void setOutputTexture(void* output) { outputTexture_ = output; }

    /** SSR参数 */
    void setMaxIterations(int iterations) { maxIterations_ = iterations; }
    void setMaxStep(float step) { maxStep_ = step; }
    void setThickness(float thickness) { thickness_ = thickness; }
    void setRoughnessThreshold(float threshold) { roughnessThreshold_ = threshold; }

private:
    void* colorTexture_ = nullptr;
    void* depthTexture_ = nullptr;
    void* normalTexture_ = nullptr;
    void* outputTexture_ = nullptr;

    // SSR参数
    int maxIterations_ = 128;
    float maxStep_ = 0.05f;
    float thickness_ = 0.01f;
    float roughnessThreshold_ = 0.5f;
};

// ============================================================================
// Motion Blur
// ============================================================================

/**
 * @brief 运动模糊Pass
 *
 * 基于速度向量的运动模糊
 */
class MotionBlurPass : public RenderPass {
public:
    MotionBlurPass();
    ~MotionBlurPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setColorTexture(void* color) { colorTexture_ = color; }
    void setVelocityTexture(void* velocity) { velocityTexture_ = velocity; }
    void setDepthTexture(void* depth) { depthTexture_ = depth; }
    void setOutputTexture(void* output) { outputTexture_ = output; }

    /** 运动模糊参数 */
    void setIntensity(float intensity) { intensity_ = intensity; }
    void setSampleCount(int count) { sampleCount_ = count; }

private:
    void* colorTexture_ = nullptr;
    void* velocityTexture_ = nullptr;
    void* depthTexture_ = nullptr;
    void* outputTexture_ = nullptr;

    float intensity_ = 0.5f;
    int sampleCount_ = 16;
};

// ============================================================================
// Depth of Field
// ============================================================================

/**
 * @brief 景深Pass
 *
 * 模拟相机景深效果
 */
class DepthOfFieldPass : public RenderPass {
public:
    DepthOfFieldPass();
    ~DepthOfFieldPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setColorTexture(void* color) { colorTexture_ = color; }
    void setDepthTexture(void* depth) { depthTexture_ = depth; }
    void setOutputTexture(void* output) { outputTexture_ = output; }

    /** 景深参数 */
    void setFocusDistance(float distance) { focusDistance_ = distance; }
    void setAperture(float aperture) { aperture_ = aperture; }
    void setFocalLength(float length) { focalLength_ = length; }
    void setMaxBlurSize(float size) { maxBlurSize_ = size; }

private:
    void* colorTexture_ = nullptr;
    void* depthTexture_ = nullptr;
    void* outputTexture_ = nullptr;

    float focusDistance_ = 10.0f;
    float aperture_ = 5.6f;
    float focalLength_ = 50.0f;
    float maxBlurSize_ = 5.0f;
};

// ============================================================================
// Screen Space Global Illumination (实验性)
// ============================================================================

/**
 * @brief SSGI Pass
 *
 * 屏幕空间全局光照（实验性）
 */
class SSGIPass : public RenderPass {
public:
    SSGIPass();
    ~SSGIPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setColorTexture(void* color) { colorTexture_ = color; }
    void setDepthTexture(void* depth) { depthTexture_ = depth; }
    void setNormalTexture(void* normal) { normalTexture_ = normal; }
    void setOutputTexture(void* output) { outputTexture_ = output; }

    /** SSGI参数 */
    void setSampleCount(int count) { sampleCount_ = count; }
    void setRadius(float radius) { radius_ = radius; }
    void setIntensity(float intensity) { intensity_ = intensity; }

private:
    void* colorTexture_ = nullptr;
    void* depthTexture_ = nullptr;
    void* normalTexture_ = nullptr;
    void* outputTexture_ = nullptr;

    int sampleCount_ = 256;
    float radius_ = 0.2f;
    float intensity_ = 0.5f;
};
