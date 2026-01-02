/**
 * @file VolumetricFeature.h
 * @brief 体积光/雾特效
 */

#pragma once

#include "../IRenderFeature.h"
#include "../../RenderPass.h"

/**
 * @brief 体积光Feature（God Rays）
 */
class VolumetricLightFeature : public IRenderFeature {
public:
    VolumetricLightFeature();
    ~VolumetricLightFeature() override;

    bool Initialize(IRenderContext& context) override;
    void Cleanup() override;
    void AddRenderPasses(BasicRenderer& renderer) override;
    void Execute(IRenderContext& context, const RenderingData& renderingData) override;

    void SetLightPosition(float3 pos) { lightPosition_ = pos; }
    void SetIntensity(float intensity) { intensity_ = intensity; }
    void SetSamples(int samples) { samples_ = samples; }

private:
    float3 lightPosition_ = {0, 10, 0};
    float intensity_ = 0.5f;
    int samples_ = 64;

    class GodRaysPass* godRaysPass_ = nullptr;
};

/**
 * @brief 体积雾Feature
 */
class VolumetricFogFeature : public IRenderFeature {
public:
    VolumetricFogFeature();
    ~VolumetricFogFeature() override;

    bool Initialize(IRenderContext& context) override;
    void Cleanup() override;
    void AddRenderPasses(BasicRenderer& renderer) override;
    void Execute(IRenderContext& context, const RenderingData& renderingData) override;

    void SetDensity(float density) { density_ = density; }
    void SetHeight(float height) { height_ = height; }
    void SetHeightFalloff(float falloff) { heightFalloff_ = falloff; }

private:
    float density_ = 0.01f;
    float height_ = 50.0f;
    float heightFalloff_ = 0.1f;

    class VolumetricFogPass* fogPass_ = nullptr;
};

/**
 * @brief 体积云Feature
 */
class VolumetricCloudFeature : public IRenderFeature {
public:
    VolumetricCloudFeature();
    ~VolumetricCloudFeature() override;

    bool Initialize(IRenderContext& context) override;
    void Cleanup() override;
    void AddRenderPasses(BasicRenderer& renderer) override;
    void Execute(IRenderContext& context, const RenderingData& renderingData) override;

    void SetCloudCoverage(float coverage) { coverage_ = coverage; }
    void SetCloudDensity(float density) { density_ = density; }
    void SetCloudHeight(float height) { height_ = height; }

private:
    float coverage_ = 0.5f;
    float density_ = 0.8f;
    float height_ = 1500.0f;

    class VolumetricCloudPass* cloudPass_ = nullptr;
};
