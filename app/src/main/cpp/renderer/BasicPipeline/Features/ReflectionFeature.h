/**
 * @file ReflectionFeature.h
 * @brief 反射特效（反射探针、平面反射）
 */

#pragma once

#include "../IRenderFeature.h"
#include "../../RenderPass.h"
#include <vector>

/**
 * @brief 反射探针数据
 */
struct ReflectionProbe {
    float3 position;
    float influenceRadius;
    void* cubemap;
    int resolution;
};

/**
 * @brief 反射探针Feature
 */
class ReflectionProbeFeature : public IRenderFeature {
public:
    ReflectionProbeFeature();
    ~ReflectionProbeFeature() override;

    bool Initialize(IRenderContext& context) override;
    void Cleanup() override;
    void AddRenderPasses(BasicRenderer& renderer) override;
    void Execute(IRenderContext& context, const RenderingData& renderingData) override;

    void SetProbes(const std::vector<ReflectionProbe>& probes) { probes_ = probes; }
    void AddProbe(const ReflectionProbe& probe) { probes_.push_back(probe); }

private:
    std::vector<ReflectionProbe> probes_;
    class ReflectionBlendPass* blendPass_ = nullptr;
};

/**
 * @brief 平面反射Feature
 *
 * 用于镜子、水面等平面反射
 */
class PlanarReflectionFeature : public IRenderFeature {
public:
    PlanarReflectionFeature();
    ~PlanarReflectionFeature() override;

    bool Initialize(IRenderContext& context) override;
    void Cleanup() override;
    void AddRenderPasses(BasicRenderer& renderer) override;
    void Execute(IRenderContext& context, const RenderingData& renderingData) override;

    void SetReflectionPlane(float4 plane) { reflectionPlane_ = plane; }

private:
    float4 reflectionPlane_ = {0, 1, 0, 0};  // 默认XZ平面
    class PlanarReflectionPass* reflectionPass_ = nullptr;
};
