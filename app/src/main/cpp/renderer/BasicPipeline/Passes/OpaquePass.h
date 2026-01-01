/**
 * @file OpaquePass.h
 * @brief 不透明物体渲染通道
 *
 * 使用PBR光照模型渲染不透明物体
 *
 * 渲染顺序: 从前到后（利用Early-Z优化）
 * 深度: 测试开启，写入开启
 */

#pragma once

#include "../../RenderPass.h"
#include "../RenderingData.h"
#include "../RenderQueue.h"
#include "../LightingData.h"
#include "../DepthState.h"
#include <memory>

/**
 * @brief 不透明物体渲染Pass
 *
 * 功能:
 * - 渲染所有不透明物体
 * - PBR光照计算
 * - 阴影接收
 * - 从前到后排序优化
 */
class OpaquePass : public RenderPass {
public:
    OpaquePass();
    ~OpaquePass() override = default;

    // ========================================================================
    // RenderPass 接口
    // ========================================================================

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    // ========================================================================
    // 配置
    // ========================================================================

    void setRenderQueue(RenderQueue* queue) { renderQueue_ = queue; }
    void setRenderingData(const RenderingData* data) { renderingData_ = data; }
    void setLightingData(const LightingData* data) { lightingData_ = data; }
    void setShadowMapArray(void* shadowMap) { shadowMapArray_ = shadowMap; }
    void setShadowMatrices(const Matrix4* matrices, size_t count);

    // ========================================================================
    // 光照开关（调试用）
    // ========================================================================

    void setEnableDirectLighting(bool enable) { enableDirectLighting_ = enable; }
    void setEnableIndirectLighting(bool enable) { enableIndirectLighting_ = enable; }
    void setEnableShadows(bool enable) { enableShadows_ = enable; }

private:
    RenderQueue* renderQueue_ = nullptr;
    const RenderingData* renderingData_ = nullptr;
    const LightingData* lightingData_ = nullptr;
    void* shadowMapArray_ = nullptr;
    std::vector<Matrix4> shadowMatrices_;

    // 光照开关
    bool enableDirectLighting_ = true;
    bool enableIndirectLighting_ = false;
    bool enableShadows_ = true;

    void createPipelines(VkDevice device, VkRenderPass renderPass);
};
