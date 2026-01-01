/**
 * @file ShadowPass.h
 * @brief 阴影渲染通道
 *
 * 功能:
 * - 渲染定向光的阴影贴图
 * - 渲染点光源的立方体阴影贴图
 * - 渲染聚光灯的阴影贴图
 * - 支持级联阴影贴图 (CSM)
 */

#pragma once

#include "../RenderQueue.h"
#include "../ShadowSettings.h"
#include "../../RenderPass.h"
#include <vector>
#include <memory>

// 前向声明
class LightData;
class RenderingData;

/**
 * @brief 阴影渲染通道
 *
 * 负责渲染所有光源的阴影贴图
 *
 * 渲染流程:
 * 1. 对于每个投射阴影的光源:
 *    a. 设置阴影贴图为渲染目标
 *    b. 从光源视角渲染场景
 *    c. 应用深度偏移避免阴影痤疮
 * 2. 将阴影贴图传递给光照Pass
 */
class ShadowPass : public RenderPass {
public:
    ShadowPass();
    ~ShadowPass() override = default;

    // ========================================================================
    // RenderPass 接口实现
    // ========================================================================

    /**
     * @brief 初始化阴影通道
     *
     * 创建阴影贴图资源、帧缓冲等
     */
    void initialize(VkDevice device, VkRenderPass renderPass) override;

    /**
     * @brief 记录阴影渲染命令
     *
     * @param cmdBuffer 命令缓冲区
     */
    void record(VkCommandBuffer cmdBuffer) override;

    /**
     * @brief 清理阴影资源
     */
    void cleanup(VkDevice device) override;

    // ========================================================================
    // 配置
    // ========================================================================

    /**
     * @brief 设置阴影设置
     */
    void setShadowSettings(const ShadowSettings* settings);

    /**
     * @brief 设置渲染队列管理器
     *
     * 用于获取需要渲染阴影的对象
     */
    void setRenderQueueManager(RenderQueueManager* queueManager);

    /**
     * @brief 设置要渲染的光源列表
     */
    void setLights(const std::vector<LightData>* lights);

    // ========================================================================
    // 级联阴影 (CSM)
    // ========================================================================

    /**
     * @brief 计算级联阴影的分割距离
     *
     * @param nearPlane 近平面距离
     * @param farPlane 远平面距离
     * @return 分割距离数组
     */
    std::vector<float> calculateCascadeSplits(float nearPlane, float farPlane) const;

    /**
     * @brief 计算级联阴影的正交投影矩阵
     *
     * @param cascadeIndex 级联索引
     * @param splitDistances 分割距离
     * @param viewMatrix 相机视图矩阵
     * @param projMatrix 相机投影矩阵
     * @return 级联的投影矩阵
     */
    Matrix4 calculateCascadeProjection(int cascadeIndex,
                                       const std::vector<float>& splitDistances,
                                       const Matrix4& viewMatrix,
                                       const Matrix4& projMatrix) const;

    // ========================================================================
    // 资源访问
    // ========================================================================

    /**
     * @brief 获取阴影贴图数组（用于光照Pass）
     *
     * @return 阴影贴图句柄（API相关）
     */
    void* getShadowMapArray() const { return shadowMapArray_; }

    /**
     * @brief 获取阴影矩阵数组（用于光照Pass）
     *
     * 每个光源的阴影矩阵: ShadowMatrix = Projection * View * Bias
     *
     * @return 阴影矩阵列表
     */
    const std::vector<Matrix4>& getShadowMatrices() const { return shadowMatrices_; }

    // ========================================================================
    // 调试
    // ========================================================================

    /**
     * @brief 是否启用阴影可视化调试
     */
    void setDebugVisualization(bool enable) { debugVisualization_ = enable; }

private:
    // ========================================================================
    // 成员变量
    // ========================================================================

    /** 阴影设置 */
    const ShadowSettings* shadowSettings_ = nullptr;

    /** 渲染队列管理器 */
    RenderQueueManager* queueManager_ = nullptr;

    /** 光源列表 */
    const std::vector<LightData>* lights_ = nullptr;

    // ========================================================================
    // 阴影贴图资源
    // ========================================================================

    /** 阴影贴图数组（所有光源共享） */
    void* shadowMapArray_ = nullptr;  // VkImage

    /** 阴影贴图视图 */
    void* shadowMapView_ = nullptr;   // VkImageView

    /** 阴影贴图采样器 */
    void* shadowSampler_ = nullptr;   // VkSampler

    /** 阴影帧缓冲 */
    std::vector<void*> shadowFramebuffers_;  // VkFramebuffer

    // ========================================================================
    // 级联阴影资源
    // ========================================================================

    /** 级联阴影贴图视图 */
    std::vector<void*> cascadeViews_;  // VkImageView

    /** 级联帧缓冲 */
    std::vector<void*> cascadeFramebuffers_;  // VkFramebuffer

    // ========================================================================
    // 渲染数据
    // ========================================================================

    /** 阴影矩阵列表（每个光源一个） */
    std::vector<Matrix4> shadowMatrices_;

    /** 光源空间变换矩阵（视图矩阵） */
    std::vector<Matrix4> lightViewMatrices_;

    /** 光源投影矩阵 */
    std::vector<Matrix4> lightProjMatrices_;

    // ========================================================================
    // 调试
    // ========================================================================

    /** 是否启用阴影可视化 */
    bool debugVisualization_ = false;

    // ========================================================================
    // 辅助方法
    // ========================================================================

    /**
     * @brief 渲染定向光阴影
     */
    void renderDirectionalLightShadows(VkCommandBuffer cmdBuffer, const LightData& light);

    /**
     * @brief 渲染点光源阴影（立方体贴图）
     */
    void renderPointLightShadows(VkCommandBuffer cmdBuffer, const LightData& light);

    /**
     * @brief 渲染聚光灯阴影
     */
    void renderSpotLightShadows(VkCommandBuffer cmdBuffer, const LightData& light);

    /**
     * @brief 渲染级联阴影
     */
    void renderCascadedShadows(VkCommandBuffer cmdBuffer, const LightData& light);

    /**
     * @brief 创建深度偏移矩阵（避免Shadow Acne）
     *
     * 将 [0, 1] 深度范围映射到光源空间的正确范围
     *
     * @return 偏移矩阵
     */
    Matrix4 createDepthBiasMatrix() const;

    /**
     * @brief 应用深度偏移到投影矩阵
     *
     * @param proj 原始投影矩阵
     * @param constantBias 常量偏移
     * @param slopeBias 斜率偏移
     * @return 修改后的投影矩阵
     */
    Matrix4 applyDepthBias(const Matrix4& proj, float constantBias, float slopeBias) const;
};

/**
 * @brief 阴影渲染器接口
 *
 * 用于不同API的阴影渲染实现
 */
class IShadowRenderer {
public:
    virtual ~IShadowRenderer() = default;

    /**
     * @brief 初始化阴影渲染器
     */
    virtual bool initialize() = 0;

    /**
     * @brief 渲染阴影
     */
    virtual void renderShadowMap(const LightData& light,
                                  const std::vector<RenderObject>& objects,
                                  const Matrix4& lightViewMatrix,
                                  const Matrix4& lightProjMatrix) = 0;

    /**
     * @brief 清理资源
     */
    virtual void cleanup() = 0;
};
