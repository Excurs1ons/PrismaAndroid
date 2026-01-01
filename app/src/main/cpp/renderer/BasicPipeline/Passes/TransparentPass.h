/**
 * @file TransparentPass.h
 * @brief 透明物体渲染通道
 *
 * 功能:
 * - 渲染半透明物体
 * - 正确的透明混合
 * - 从后到前排序
 * - 支持Alpha Test和Alpha Blend
 */

#pragma once

#include "../RenderQueue.h"
#include "../DepthState.h"
#include "../../RenderPass.h"
#include <vector>
#include <memory>

// 前向声明
class RenderingData;
class Material;

/**
 * @brief 透明渲染模式
 */
enum class TransparentMode {
    /** Alpha混合（标准透明） */
    AlphaBlend = 0,

    /** 加法混合（发光效果） */
    Additive = 1,

    /** 乘法混合（ darken 效果） */
    Multiply = 2,

    /** 自定义混合 */
    Custom = 3
};

/**
 * @brief Alpha混合设置
 */
struct AlphaBlendSettings {
    /** 源颜色混合因子 */
    BlendFactor srcColor = BlendFactor::SrcAlpha;

    /** 目标颜色混合因子 */
    BlendFactor dstColor = BlendFactor::OneMinusSrcAlpha;

    /** 颜色混合操作 */
    BlendOp colorOp = BlendOp::Add;

    /** 源Alpha混合因子 */
    BlendFactor srcAlpha = BlendFactor::One;

    /** 目标Alpha混合因子 */
    BlendFactor dstAlpha = BlendFactor::Zero;

    /** Alpha混合操作 */
    BlendOp alphaOp = BlendOp::Add;

    // ========================================================================
    // 预设
    // ========================================================================

    /**
     * @brief 标准Alpha混合
     *
     * Final = Src * SrcAlpha + Dst * (1 - SrcAlpha)
     */
    static AlphaBlendSettings standard() {
        AlphaBlendSettings settings;
        settings.srcColor = BlendFactor::SrcAlpha;
        settings.dstColor = BlendFactor::OneMinusSrcAlpha;
        settings.colorOp = BlendOp::Add;
        settings.srcAlpha = BlendFactor::One;
        settings.dstAlpha = BlendFactor::Zero;
        settings.alphaOp = BlendOp::Add;
        return settings;
    }

    /**
     * @brief 加法混合（发光、火焰）
     *
     * Final = Src * SrcAlpha + Dst
     */
    static AlphaBlendSettings additive() {
        AlphaBlendSettings settings;
        settings.srcColor = BlendFactor::SrcAlpha;
        settings.dstColor = BlendFactor::One;
        settings.colorOp = BlendOp::Add;
        settings.srcAlpha = BlendFactor::One;
        settings.dstAlpha = BlendFactor::Zero;
        settings.alphaOp = BlendOp::Add;
        return settings;
    }

    /**
     * @brief 乘法混合（阴影、 darken）
     *
     * Final = Src * Dst
     */
    static AlphaBlendSettings multiply() {
        AlphaBlendSettings settings;
        settings.srcColor = BlendFactor::DstColor;
        settings.dstColor = BlendFactor::Zero;
        settings.colorOp = BlendOp::Add;
        settings.srcAlpha = BlendFactor::One;
        settings.dstAlpha = BlendFactor::Zero;
        settings.alphaOp = BlendOp::Add;
        return settings;
    }

    /**
     * @brief 预乘Alpha混合
     *
     * 颜色已预先乘以Alpha值
     * Final = Src + Dst * (1 - SrcAlpha)
     */
    static AlphaBlendSettings premultiplied() {
        AlphaBlendSettings settings;
        settings.srcColor = BlendFactor::One;
        settings.dstColor = BlendFactor::OneMinusSrcAlpha;
        settings.colorOp = BlendOp::Add;
        settings.srcAlpha = BlendFactor::One;
        settings.dstAlpha = BlendFactor::Zero;
        settings.alphaOp = BlendOp::Add;
        return settings;
    }
};

/**
 * @brief 透明物体渲染通道
 *
 * 渲染所有半透明物体
 *
 * 渲染顺序:
 * 1. 从后到前排序（确保正确的颜色混合）
 * 2. 渲染每个透明物体
 * 3. 应用深度测试但不写入深度
 *
 * 重要:
 * - 必须在不透明物体之后渲染
 * - 必须从后到前排序
 * - 深度测试开启，深度写入关闭
 */
class TransparentPass : public RenderPass {
public:
    TransparentPass();
    ~TransparentPass() override = default;

    // ========================================================================
    // RenderPass 接口实现
    // ========================================================================

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    // ========================================================================
    // 配置
    // ========================================================================

    /**
     * @brief 设置透明渲染队列
     *
     * 队列中的物体应该已经按从后到前排序
     */
    void setRenderQueue(RenderQueue* queue);

    /**
     * @brief 设置渲染数据
     */
    void setRenderingData(const RenderingData* data);

    // ========================================================================
    // 混合模式
    // ========================================================================

    /**
     * @brief 设置全局混合模式
     *
     * 所有透明物体将使用此混合模式
     * 也可以在每个材质上单独设置
     */
    void setBlendMode(TransparentMode mode);

    /**
     * @brief 设置自定义混合设置
     */
    void setBlendSettings(const AlphaBlendSettings& settings);

    // ========================================================================
    // 调试
    // ========================================================================

    /**
     * @brief 是否启用透明物体包围盒渲染
     */
    void setDebugBounds(bool enable) { debugBounds_ = enable; }

private:
    // ========================================================================
    // 成员变量
    // ========================================================================

    /** 透明渲染队列 */
    RenderQueue* renderQueue_ = nullptr;

    /** 渲染数据 */
    const RenderingData* renderingData_ = nullptr;

    /** 当前混合模式 */
    TransparentMode blendMode_ = TransparentMode::AlphaBlend;

    /** 混合设置 */
    AlphaBlendSettings blendSettings_;

    /** 调试选项 */
    bool debugBounds_ = false;

    // ========================================================================
    // 辅助方法
    // ========================================================================

    /**
     * @brief 渲染单个透明物体
     */
    void renderTransparentObject(VkCommandBuffer cmdBuffer, const RenderObject& obj);

    /**
     * @brief 应用混合设置到管线
     *
     * @param pipeline 管线句柄
     * @param settings 混合设置
     */
    void applyBlendSettings(VkPipeline pipeline, const AlphaBlendSettings& settings);

    /**
     * @brief 创建透明渲染管线
     *
     * 透明管线的特点:
     * - 深度测试开启
     * - 深度写入关闭
     * - Alpha混合开启
     */
    void createTransparentPipeline(VkDevice device, VkRenderPass renderPass);
};

/**
 * @brief Alpha测试通道
 *
 * 在透明队列之前渲染
 * 使用discard()剔除透明像素，但保留深度写入
 *
 * 适用场景:
 * - 栅栏、树叶等带有透明孔洞的不透明物体
 * - 需要正确深度遮挡的物体
 */
class AlphaTestPass : public RenderPass {
public:
    AlphaTestPass();
    ~AlphaTestPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    /**
     * @brief 设置Alpha测试队列
     */
    void setRenderQueue(RenderQueue* queue);

    /**
     * @brief 设置Alpha测试阈值
     *
     * 像素Alpha < threshold 时被丢弃
     */
    void setAlphaThreshold(float threshold) { alphaThreshold_ = threshold; }

private:
    RenderQueue* renderQueue_ = nullptr;
    float alphaThreshold_ = 0.5f;

    void createAlphaTestPipeline(VkDevice device, VkRenderPass renderPass);
};

/**
 * @brief 粒子渲染通道
 *
 * 专门用于粒子系统的渲染
 * 支持GPU粒子实例化
 */
class ParticlePass : public RenderPass {
public:
    ParticlePass();
    ~ParticlePass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    /**
     * @brief 设置粒子数据
     */
    struct ParticleData {
        Vector3 position;
        Vector3 velocity;
        Vector4 color;
        float size;
        float life;
        float maxLife;
    };
    void setParticles(const std::vector<ParticleData>& particles);

    /**
     * @brief 设置粒子纹理
     */
    void setParticleTexture(void* textureHandle);

private:
    std::vector<ParticleData> particles_;
    void* particleTexture_ = nullptr;

    void createParticlePipeline(VkDevice device, VkRenderPass renderPass);
};
