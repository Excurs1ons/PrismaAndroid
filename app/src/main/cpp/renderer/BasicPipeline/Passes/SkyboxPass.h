/**
 * @file SkyboxPass.h
 * @brief 天空盒渲染通道
 *
 * 功能:
 * - 渲染天空盒/天空球
 * - 渐变天空
 * - 程序化云朵
 * - 环境反射
 */

#pragma once

#include "../../RenderPass.h"
#include "../RenderingData.h"
#include <memory>

/**
 * @brief 天空盒模式
 */
enum class SkyboxMode {
    /** 静态立方体贴图 */
    Cubemap,

    /** 程序化渐变天空 */
    Procedural,

    /** 预积分大气散射 */
    AtmosphericScattering,

    /** 无天空盒 */
    None
};

/**
 * @brief 大气散射设置
 */
struct AtmosphereSettings {
    float rayleighScattering = 0.005f;  // 瑞利散射系数
    float mieScattering = 0.002f;       // 米氏散射系数
    float mieG = 0.8f;                  // 米氏相位函数参数
    float height = 1000.0f;             // 大气层高度
    float3 sunDirection = {0.5f, 0.2f, 0.2f};
    float sunIntensity = 1.0f;
};

/**
 * @brief 天空盒Pass
 */
class SkyboxPass : public RenderPass {
public:
    SkyboxPass();
    ~SkyboxPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    // ========================================================================
    // 配置
    // ========================================================================

    void setCamera(void* camera) { camera_ = camera; }
    void setRenderingData(const RenderingData* data) { renderingData_ = data; }
    void setSkyboxMode(SkyboxMode mode) { mode_ = mode; }

    /** 设置立方体贴图天空盒 */
    void setCubemap(void* cubemapTexture) { cubemapTexture_ = cubemapTexture; }

    /** 设置渐变天空颜色 */
    void setGradientColors(const float3& top, const float3& bottom, const float3& horizon) {
        gradientTop_ = top;
        gradientBottom_ = bottom;
        gradientHorizon_ = horizon;
    }

    /** 设置大气散射参数 */
    void setAtmosphereSettings(const AtmosphereSettings& settings) {
        atmosphereSettings_ = settings;
    }

    /** 获取环境纹理（用于反射） */
    void* getEnvironmentTexture() const { return environmentTexture_; }

private:
    void* camera_ = nullptr;
    const RenderingData* renderingData_ = nullptr;
    SkyboxMode mode_ = SkyboxMode::Cubemap;

    // Cubemap模式
    void* cubemapTexture_ = nullptr;
    void* environmentTexture_ = nullptr;

    // 渐变模式
    float3 gradientTop_ = {0.1f, 0.4f, 0.8f};
    float3 gradientBottom_ = {0.6f, 0.7f, 0.9f};
    float3 gradientHorizon_ = {0.8f, 0.85f, 0.95f};

    // 大气散射模式
    AtmosphereSettings atmosphereSettings_;

    void createCubemapPipeline(VkDevice device, VkRenderPass renderPass);
    void createProceduralPipeline(VkDevice device, VkRenderPass renderPass);
    void createAtmosphericPipeline(VkDevice device, VkRenderPass renderPass);
};

/**
 * @brief 云朵渲染Pass
 *
 * 程序化体积云
 */
class CloudPass : public RenderPass {
public:
    CloudPass();
    ~CloudPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setCamera(void* camera) { camera_ = camera; }
    void setTimeOfDay(float time) { timeOfDay_ = time; }

    struct CloudSettings {
        float coverage = 0.5f;      // 云覆盖率
        float density = 0.8f;       // 云密度
        float absorption = 0.3f;    // 光吸收
        float scattering = 0.6f;    // 光散射
        float height = 1500.0f;     // 云层高度
        float thickness = 500.0f;   // 云层厚度
    };
    void setSettings(const CloudSettings& settings) { settings_ = settings; }

private:
    void* camera_ = nullptr;
    float timeOfDay_ = 0.5f;  // 0=午夜, 0.5=正午, 1=午夜
    CloudSettings settings_;
};

/**
 * @brief 太阳/月亮渲染Pass
 */
class CelestialBodyPass : public RenderPass {
public:
    CelestialBodyPass();
    ~CelestialBodyPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setSunDirection(const float3& dir) { sunDirection_ = dir; }
    void setSunColor(const float3& color) { sunColor_ = color; }
    void setSunSize(float size) { sunSize_ = size; }

    void setMoonDirection(const float3& dir) { moonDirection_ = dir; }
    void setMoonColor(const float3& color) { moonColor_ = color; }
    void setMoonSize(float size) { moonSize_ = size; }

private:
    float3 sunDirection_ = {0, 1, 0};
    float3 sunColor_ = {1, 0.95f, 0.8f};
    float sunSize_ = 0.02f;

    float3 moonDirection_ = {0, -1, 0};
    float3 moonColor_ = {0.7f, 0.7f, 0.8f};
    float moonSize_ = 0.015f;
};
