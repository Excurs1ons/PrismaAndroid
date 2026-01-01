/**
 * @file BasicPipelineRenderer.h
 * @brief 基础渲染管线控制器
 *
 * 协调所有渲染通道的执行，管理渲染流程
 *
 * 渲染流程:
 * 1. Setup - 收集渲染数据
 * 2. ShadowPass - 渲染阴影
 * 3. OpaquePass - 渲染不透明物体
 * 4. SkyboxPass - 渲染天空盒
 * 5. TransparentPass - 渲染透明物体
 * 6. PostProcess - 后处理
 *
 * 支持通过ScriptableRenderFeature在任意位置插入自定义Pass
 */

#pragma once

#include "ScriptableRenderFeature.h"
#include "RenderingData.h"
#include "Camera.h"
#include "RenderQueue.h"
#include "ShadowSettings.h"
#include "LightingData.h"
#include "../RenderPass.h"
#include <memory>
#include <vector>
#include <unordered_map>

// 前向声明
class Scene;

/**
 * @brief 渲染管线配置
 */
struct PipelineConfig {
    /** 是否启用阴影 */
    bool enableShadows = true;

    /** 是否启用后处理 */
    bool enablePostProcessing = true;

    /** 是否启用天空盒 */
    bool enableSkybox = true;

    /** 渲染分辨率比例 (1.0 = 全分辨率) */
    float renderScale = 1.0f;

    /** MSAA样本数 */
    uint32_t msaaSamples = 1;

    /** 色调映射模式 */
    enum class ToneMappingMode {
        None,
        ACES,
        Reinhard,
        Linear
    };
    ToneMappingMode toneMapping = ToneMappingMode::ACES;

    /** 伽马校正 */
    bool enableGammaCorrection = true;
};

/**
 * @brief 基础渲染管线
 *
 * 实现完整的渲染流程
 */
class BasicPipelineRenderer : public ScriptableRenderer {
public:
    BasicPipelineRenderer();
    ~BasicPipelineRenderer() override;

    // ========================================================================
    // 初始化
    // ========================================================================

    /**
     * @brief 初始化渲染管线
     *
     * @param device API设备
     * @param apiRenderPass API渲染通道
     */
    void initialize(VkDevice device, VkRenderPass apiRenderPass);

    /**
     * @brief 设置管线配置
     */
    void setConfig(const PipelineConfig& config);

    // ========================================================================
    // 渲染
    // ========================================================================

    /**
     * @brief 渲染场景
     *
     * @param scene 场景
     * @param camera 相机
     * @param commandBuffer 命令缓冲区
     */
    void render(Scene* scene, Camera* camera, VkCommandBuffer commandBuffer);

    /**
     * @brief 设置渲染数据
     */
    void setRenderingData(const RenderingData& data);

    // ========================================================================
    // ScriptableRenderer 接口实现
    // ========================================================================

    void* getCommandBuffer() override { return currentCommandBuffer_; }
    void* getRenderTarget(RenderTargetIdentifier id) override;
    void* createTemporaryRenderTexture(const RenderTextureDescriptor& desc) override;
    void releaseTemporaryRenderTexture(void* texture) override;
    void drawFullScreen(void* pipeline) override;
    void drawProcedural(void* pipeline, uint32_t vertexCount) override;
    void executeCommandBuffer(void* cmdBuffer) override;
    void* getAPIDevice() override { return device_; }
    void* getAPIRenderPass() override { return apiRenderPass_; }

    // ========================================================================
    // 渲染特性管理
    // ========================================================================

    /**
     * @brief 添加渲染特性
     */
    void addRenderFeature(std::unique_ptr<ScriptableRenderFeature> feature);

    /**
     * @brief 获取渲染特性管理器
     */
    RenderFeatureManager& getFeatureManager() { return featureManager_; }

    // ========================================================================
    // Pass访问
    // ========================================================================

    /** 阴影Pass */
    class ShadowPass* getShadowPass() { return shadowPass_.get(); }

    /** 不透明Pass */
    class OpaquePass* getOpaquePass() { return opaquePass_.get(); }

    /** 透明Pass */
    class TransparentPass* getTransparentPass() { return transparentPass_.get(); }

    /** 渲染队列管理器 */
    RenderQueueManager& getQueueManager() { return queueManager_; }

    // ========================================================================
    // 资源管理
    // ========================================================================

    /**
     * @brief 清理资源
     */
    void cleanup();

    /**
     * @brief 设置帧索引
     */
    void setCurrentFrame(uint32_t frame) { currentFrame_ = frame; }

private:
    // ========================================================================
    // 渲染阶段
    // ========================================================================

    /**
     * @brief 准备阶段
     *
     * - 构建渲染队列
     * - 视锥剔除
     * - 计算光照数据
     */
    void prepareRendering();

    /**
     * @brief 渲染阴影
     */
    void renderShadows();

    /**
     * @brief 渲染不透明物体
     */
    void renderOpaques();

    /**
     * @brief 渲染天空盒
     */
    void renderSkybox();

    /**
     * @brief 渲染透明物体
     */
    void renderTransparents();

    /**
     * @brief 后处理
     */
    void renderPostProcessing();

    // ========================================================================
    // 自定义Pass执行
    // ========================================================================

    /**
     * @brief 执行指定事件点的所有自定义Pass
     */
    void executePasses(RenderPassEvent evt);

    // ========================================================================
    // 成员变量
    // ========================================================================

    // API资源
    VkDevice device_ = nullptr;
    VkRenderPass apiRenderPass_ = nullptr;
    VkCommandBuffer currentCommandBuffer_ = nullptr;

    // 配置
    PipelineConfig config_;
    uint32_t currentFrame_ = 0;

    // 渲染数据
    RenderingData renderingData_;
    Scene* scene_ = nullptr;
    Camera* camera_ = nullptr;

    // 阴影设置
    ShadowSettings shadowSettings_;

    // 光照数据
    LightingData lightingData_;

    // 渲染队列
    RenderQueueManager queueManager_;

    // 渲染特性
    RenderFeatureManager featureManager_;

    // Passes（按执行顺序）
    std::unique_ptr<class ShadowPass> shadowPass_;
    std::unique_ptr<class OpaquePass> opaquePass_;
    std::unique_ptr<class SkyboxPass> skyboxPass_;
    std::unique_ptr<class TransparentPass> transparentPass_;
    std::unique_ptr<class PostProcessPass> postProcessPass_;
    std::unique_ptr<class AlphaTestPass> alphaTestPass_;

    // 渲染目标
    struct RenderTargets {
        void* cameraColor;     // 相机颜色缓冲
        void* cameraDepth;     // 相机深度缓冲
        void* tempTexture0;    // 临时纹理0
        void* tempTexture1;    // 临时纹理1
    };
    RenderTargets renderTargets_;

    // 状态
    bool isInitialized_ = false;
};

/**
 * @brief 渲染管线工厂
 *
 * 用于创建不同配置的渲染管线
 */
class PipelineFactory {
public:
    /**
     * @brief 创建默认渲染管线
     */
    static std::unique_ptr<BasicPipelineRenderer> createDefault();

    /**
     * @brief 创建高性能渲染管线（禁用后处理）
     */
    static std::unique_ptr<BasicPipelineRenderer> createHighPerformance();

    /**
     * @brief 创建高质量渲染管线（启用所有特性）
     */
    static std::unique_ptr<BasicPipelineRenderer> createHighQuality();

    /**
     * @brief 创建移动端渲染管线
     */
    static std::unique_ptr<BasicPipelineRenderer> createMobile();
};

// ============================================================================
// 预定义的Pass实现（空白占位）
// ============================================================================

/**
 * @brief 不透明物体Pass（使用PBR着色器）
 */
class OpaquePass : public RenderPass {
public:
    OpaquePass() : RenderPass("Opaque Pass") {}
    ~OpaquePass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override {
        // TODO: 创建PBR管线
    }

    void record(VkCommandBuffer cmdBuffer) override {
        // TODO: 渲染不透明物体
        // - 遍历渲染队列
        // - 绑定材质
        // - 设置Uniform（光照数据、阴影贴图等）
        // - 绘制
    }

    void cleanup(VkDevice device) override {
        // TODO: 清理资源
    }

    void setRenderQueue(RenderQueue* queue) { renderQueue_ = queue; }
    void setRenderingData(const RenderingData* data) { renderingData_ = data; }
    void setLightingData(const LightingData* data) { lightingData_ = data; }
    void setShadowPass(class ShadowPass* pass) { shadowPass_ = pass; }

private:
    RenderQueue* renderQueue_ = nullptr;
    const RenderingData* renderingData_ = nullptr;
    const LightingData* lightingData_ = nullptr;
    class ShadowPass* shadowPass_ = nullptr;
};

/**
 * @brief 天空盒Pass
 */
class SkyboxPass : public RenderPass {
public:
    SkyboxPass() : RenderPass("Skybox Pass") {}
    ~SkyboxPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override {
        // TODO: 创建天空盒管线
    }

    void record(VkCommandBuffer cmdBuffer) override {
        // TODO: 渲染天空盒
    }

    void cleanup(VkDevice device) override {
        // TODO: 清理资源
    }

    void setCamera(Camera* camera) { camera_ = camera; }
    void setEnvironmentTexture(void* texture) { envTexture_ = texture; }

private:
    Camera* camera_ = nullptr;
    void* envTexture_ = nullptr;
};

/**
 * @brief 后处理Pass
 */
class PostProcessPass : public RenderPass {
public:
    PostProcessPass() : RenderPass("Post Process Pass") {}
    ~PostProcessPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override {
        // TODO: 创建后处理管线
        // - Bloom
        // - 色调映射
        // - 伽马校正
    }

    void record(VkCommandBuffer cmdBuffer) override {
        // TODO: 执行后处理
    }

    void cleanup(VkDevice device) override {
        // TODO: 清理资源
    }

    void setInputTexture(void* texture) { inputTexture_ = texture; }
    void setOutputTexture(void* texture) { outputTexture_ = texture; }

private:
    void* inputTexture_ = nullptr;
    void* outputTexture_ = nullptr;
};
