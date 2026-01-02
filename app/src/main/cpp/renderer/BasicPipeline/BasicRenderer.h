/**
 * @file BasicRenderer.h
 * @brief 基础渲染器 - 精简版
 *
 * 核心Pass + 可扩展的Feature系统
 *
 * 渲染流程:
 * 1. BeforeRendering Features
 * 2. ShadowPass
 * 3. BeforeRenderingOpaques Features
 * 4. OpaquePass
 * 5. AfterRenderingOpaques Features
 * 6. SkyboxPass
 * 7. BeforeRenderingTransparents Features
 * 8. TransparentPass
 * 9. AfterRenderingTransparents Features
 * 10. AfterRendering Features
 * 11. FinalBlit
 */

#pragma once

#include "IRenderFeature.h"
#include "Camera.h"
#include "RenderQueue.h"
#include "ShadowSettings.h"
#include "LightingData.h"
#include "../RenderPass.h"
#include "../Scene.h"
#include <memory>
#include <vector>

// 前向声明
class IRenderContext;

/**
 * @brief 基础渲染器配置
 */
struct RendererConfig {
    bool enableShadows = true;
    bool enablePostProcessing = true;
    bool enableSkybox = true;

    // MSAA
    uint32_t msaaSamples = 1;

    // 渲染路径
    enum class RenderPath {
        Forward,           // 前向渲染
        Deferred,          // 延迟渲染（TODO）
    };
    RenderPath renderPath = RenderPath::Forward;
};

/**
 * @brief 基础渲染器
 *
 * 实现IRenderContext接口
 */
class BasicRenderer : public IRenderContext {
public:
    BasicRenderer();
    ~BasicRenderer() override;

    // ========================================================================
    // 初始化
    // ========================================================================

    bool Initialize(VkDevice device, VkRenderPass renderPass);
    void Cleanup();

    void SetConfig(const RendererConfig& config) { config_ = config; }

    // ========================================================================
    // 渲染
    // ========================================================================

    /**
     * @brief 渲染场景
     */
    void Render(Scene* scene, Camera* camera, VkCommandBuffer cmdBuffer);

    // ========================================================================
    // IRenderContext 接口
    // ========================================================================

    void* GetCommandBuffer() override { return currentCmdBuffer_; }
    void* GetAPIDevice() override { return device_; }
    void* GetCameraColor() override;
    void* GetCameraDepth() override;
    void* CreateTemporaryTexture(uint32_t width, uint32_t height, void* format, const char* name) override;
    void ReleaseTemporaryTexture(void* texture) override;
    void DrawFullScreen(void* pipeline) override;
    void DrawProcedural(void* pipeline, uint32_t vertexCount) override;

    // ========================================================================
    // Feature管理
    // ========================================================================

    RenderFeatureManager& GetFeatureManager() { return featureManager_; }

    /**
     * @brief 添加Feature
     */
    void AddFeature(std::unique_ptr<IRenderFeature> feature);

    // ========================================================================
    // 资源访问（供Feature使用）
    // ========================================================================

    RenderQueueManager& GetQueueManager() { return queueManager_; }
    const LightingData& GetLightingData() const { return lightingData_; }
    const ShadowSettings& GetShadowSettings() const { return shadowSettings_; }
    const RenderingData& GetRenderingData() const { return renderingData_; }

    // ========================================================================
    // Pass访问（供Feature使用）
    // ========================================================================

    class OpaquePass* GetOpaquePass() { return opaquePass_.get(); }
    class TransparentPass* GetTransparentPass() { return transparentPass_.get(); }
    class SkyboxPass* GetSkyboxPass() { return skyboxPass_.get(); }
    class ShadowPass* GetShadowPass() { return shadowPass_.get(); }

private:
    // ========================================================================
    // 渲染阶段
    // ========================================================================

    void PrepareRendering();
    void ExecuteFeatures(RenderPassEvent evt);
    void RenderShadows();
    void RenderOpaques();
    void RenderSkybox();
    void RenderTransparents();
    void FinalBlit();

    // ========================================================================
    // 成员变量
    // ========================================================================

    // API
    VkDevice device_ = nullptr;
    VkRenderPass renderPass_ = nullptr;
    VkCommandBuffer currentCmdBuffer_ = nullptr;

    // 配置
    RendererConfig config_;
    uint32_t currentFrame_ = 0;

    // 场景数据
    Scene* scene_ = nullptr;
    Camera* camera_ = nullptr;
    RenderingData renderingData_;
    LightingData lightingData_;
    ShadowSettings shadowSettings_;

    // 渲染队列
    RenderQueueManager queueManager_;

    // Feature
    RenderFeatureManager featureManager_;

    // 核心Pass（只有这几个）
    std::unique_ptr<class ShadowPass> shadowPass_;
    std::unique_ptr<class OpaquePass> opaquePass_;
    std::unique_ptr<class SkyboxPass> skyboxPass_;
    std::unique_ptr<class TransparentPass> transparentPass_;
    std::unique_ptr<class FinalBlitPass> finalBlitPass_;

    // 临时纹理池
    struct TempTexture {
        void* handle;
        uint32_t width;
        uint32_t height;
        bool inUse;
    };
    std::vector<TempTexture> tempTextures_;
};

// ============================================================================
// 核心Pass定义（精简）
// ============================================================================

/**
 * @brief 不透明物体Pass
 *
 * 唯一的核心Pass，使用PBR着色器
 */
class OpaquePass : public RenderPass {
public:
    OpaquePass();
    ~OpaquePass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void SetData(const RenderingData* renderingData,
                 const LightingData* lightingData,
                 const ShadowPass* shadowPass,
                 RenderQueue* queue) {
        renderingData_ = renderingData;
        lightingData_ = lightingData;
        shadowPass_ = shadowPass;
        renderQueue_ = queue;
    }

private:
    const RenderingData* renderingData_ = nullptr;
    const LightingData* lightingData_ = nullptr;
    const ShadowPass* shadowPass_ = nullptr;
    RenderQueue* renderQueue_ = nullptr;
};

/**
 * @brief 透明物体Pass
 */
class TransparentPass : public RenderPass {
public:
    TransparentPass();
    ~TransparentPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void SetData(const RenderingData* renderingData, RenderQueue* queue) {
        renderingData_ = renderingData;
        renderQueue_ = queue;
    }

private:
    const RenderingData* renderingData_ = nullptr;
    RenderQueue* renderQueue_ = nullptr;
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

    void SetData(const Camera* camera, const RenderingData* renderingData) {
        camera_ = camera;
        renderingData_ = renderingData;
    }

    void SetEnvironmentTexture(void* cubemap) { envTexture_ = cubemap; }

private:
    const Camera* camera_ = nullptr;
    const RenderingData* renderingData_ = nullptr;
    void* envTexture_ = nullptr;
};

/**
 * @brief 阴影Pass
 */
class ShadowPass : public RenderPass {
public:
    ShadowPass();
    ~ShadowPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void SetData(const LightingData* lightingData,
                 const ShadowSettings* settings,
                 RenderQueue* queue) {
        lightingData_ = lightingData;
        settings_ = settings;
        renderQueue_ = queue;
    }

    void* GetShadowMap() const { return shadowMap_; }
    const std::vector<Matrix4>& GetShadowMatrices() const { return shadowMatrices_; }

private:
    const LightingData* lightingData_ = nullptr;
    const ShadowSettings* settings_ = nullptr;
    RenderQueue* renderQueue_ = nullptr;

    void* shadowMap_ = nullptr;
    std::vector<Matrix4> shadowMatrices_;
};

/**
 * @brief 最终输出Pass
 *
 * 将结果blit到屏幕
 */
class FinalBlitPass : public RenderPass {
public:
    FinalBlitPass();
    ~FinalBlitPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void SetSource(void* texture) { sourceTexture_ = texture; }

private:
    void* sourceTexture_ = nullptr;
};
