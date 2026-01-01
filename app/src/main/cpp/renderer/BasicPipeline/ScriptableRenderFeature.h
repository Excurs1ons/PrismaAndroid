/**
 * @file ScriptableRenderFeature.h
 * @brief 可编程渲染特性 - 即插即用的渲染功能模块
 *
 * 参考 Unity URP ScriptableRendererFeature
 * 允许在渲染管线的任意位置插入自定义渲染逻辑
 *
 * 使用场景:
 * - 后处理效果（Bloom、Motion Blur等）
 * - 自定义渲染Pass（轮廓渲染、热成像等）
 * - 调试可视化（法线显示、深度可视化等）
 * - 特殊效果（屏幕空间反射、环境光遮蔽等）
 */

#pragma once

#include "../../RenderPass.h"
#include "../RenderingData.h"
#include <string>
#include <vector>
#include <memory>

// 前向声明
class ScriptableRenderer;
class ScriptableRenderPass;

/**
 * @brief 渲染Pass事件（插入点）
 *
 * 定义在渲染管线的哪个阶段执行自定义Pass
 */
enum class RenderPassEvent {
    /** 在渲染任何内容之前（用于准备资源） */
    BeforeRendering = 0,

    /** 在阴影渲染之前 */
    BeforeRenderingShadows = 10,

    /** 在阴影渲染之后 */
    AfterRenderingShadows = 15,

    /** 在不透明物体渲染之前 */
    BeforeRenderingOpaques = 20,

    /** 在不透明物体渲染之后 */
    AfterRenderingOpaques = 25,

    /** 在天空盒渲染之前 */
    BeforeRenderingSkybox = 30,

    /** 在天空盒渲染之后 */
    AfterRenderingSkybox = 35,

    /** 在透明物体渲染之前 */
    BeforeRenderingTransparents = 40,

    /** 在透明物体渲染之后 */
    AfterRenderingTransparents = 45,

    /** 在后处理之前 */
    BeforeRenderingPostProcessing = 50,

    /** 在后处理之后 */
    AfterRenderingPostProcessing = 55,

    /** 在所有渲染之后（用于最后调整） */
    AfterRendering = 60
};

/**
 * @brief 资源标识
 *
 * 用于标识渲染目标（颜色、深度等）
 */
enum class RenderTargetIdentifier {
    /** 当前相机颜色缓冲 */
    CameraColor,

    /** 当前相机深度缓冲 */
    CameraDepth,

    /** 临时纹理1 */
    TempTexture0,

    /** 临时纹理2 */
    TempTexture1,

    /** 临时纹理3 */
    TempTexture2,

    /** 自定义纹理 */
    Custom
};

/**
 * @brief 渲染纹理描述
 *
 * 用于创建临时渲染目标
 */
struct RenderTextureDescriptor {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 1;           // 对于纹理数组
    uint32_t mipLevels = 1;       // Mip层级数
    bool enableRandomWrite = false;  // UAV/SSBO
    void* format = nullptr;       // API相关格式 (如: VK_FORMAT_R8G8B8A8_UNORM)
    const char* name = "Untitled";
    bool useDynamicResolution = false;

    /** 创建默认描述 */
    static RenderTextureDescriptor createDefault(uint32_t w, uint32_t h) {
        RenderTextureDescriptor desc;
        desc.width = w;
        desc.height = h;
        desc.depth = 1;
        desc.mipLevels = 1;
        desc.name = "TempTexture";
        return desc;
    }
};

/**
 * @brief 可编程渲染Pass
 *
 * 实际执行渲染的Pass，由ScriptableRenderFeature创建
 *
 * 设计模式:
 * - ScriptableRenderFeature: 负责创建和配置Pass
 * - ScriptableRenderPass: 负责实际的渲染执行
 */
class ScriptableRenderPass {
public:
    explicit ScriptableRenderPass(const char* name = "Unnamed Pass");
    virtual ~ScriptableRenderPass() = default;

    /**
     * @brief 配置Pass
     *
     * 在渲染前调用，用于设置输入/输出渲染目标
     *
     * @param renderingData 渲染数据
     */
    virtual void configure(const RenderingData& renderingData);

    /**
     * @brief 设置渲染目标
     *
     * 指定此Pass的输入和输出渲染目标
     *
     * @param colorTarget 颜色渲染目标
     * @param depthTarget 深度渲染目标（可选）
     */
    void ConfigureOutput(RenderTargetIdentifier colorTarget,
                          RenderTargetIdentifier depthTarget = RenderTargetIdentifier::CameraDepth);

    /**
     * @brief 配置临时纹理需求
     *
     * @param descriptor 纹理描述
     * @param identifier 纹理标识
     */
    void ConfigureTempSurface(const RenderTextureDescriptor& descriptor,
                               RenderTargetIdentifier identifier);

    /**
     * @brief 执行渲染
     *
     * @param context 渲染上下文
     * @param renderingData 渲染数据
     */
    virtual void execute(ScriptableRenderer& context,
                         const RenderingData& renderingData) = 0;

    /**
     * @brief 创建渲染资源
     *
     * 在Pass第一次执行前调用
     */
    virtual void createResources() {}

    /**
     * @brief 释放渲染资源
     */
    virtual void releaseResources() {}

    /**
     * @brief 帧开始时调用
     */
    virtual void onFrameStart() {}

    /**
     * @brief 帧结束时调用
     */
    virtual void onFrameEnd() {}

    // ========================================================================
    // 访问器
    // ========================================================================

    const char* GetName() const { return name_; }
    RenderPassEvent GetRenderPassEvent() const { renderPassEvent_; }
    void SetRenderPassEvent(RenderPassEvent evt) { renderPassEvent_ = evt; }

    /** 获取颜色渲染目标 */
    RenderTargetIdentifier GetColorTarget() const { return colorTarget_; }

    /** 获取深度渲染目标 */
    RenderTargetIdentifier GetDepthTarget() const { return depthTarget_; }

    /** 是否需要深度渲染目标 */
    bool RequiresDepth() const { return requiresDepth_; }

protected:
    const char* name_;
    RenderPassEvent renderPassEvent_ = RenderPassEvent::AfterRenderingOpaques;
    RenderTargetIdentifier colorTarget_ = RenderTargetIdentifier::CameraColor;
    RenderTargetIdentifier depthTarget_ = RenderTargetIdentifier::CameraDepth;
    bool requiresDepth_ = false;

    // 临时纹理需求
    struct TempSurfaceRequest {
        RenderTextureDescriptor descriptor;
        RenderTargetIdentifier identifier;
    };
    std::vector<TempSurfaceRequest> tempSurfaceRequests_;
};

/**
 * @brief 渲染上下文
 *
 * 提供Pass执行所需的API接口
 */
class ScriptableRenderer {
public:
    virtual ~ScriptableRenderer() = default;

    /**
     * @brief 获取命令缓冲区
     */
    virtual void* getCommandBuffer() = 0;

    /**
     * @brief 获取渲染目标
     */
    virtual void* getRenderTarget(RenderTargetIdentifier id) = 0;

    /**
     * @brief 创建临时渲染目标
     */
    virtual void* createTemporaryRenderTexture(const RenderTextureDescriptor& desc) = 0;

    /**
     * @brief 释放临时渲染目标
     */
    virtual void releaseTemporaryRenderTexture(void* texture) = 0;

    /**
     * @brief 绘制全屏三角形
     *
     * 用于后处理效果
     */
    virtual void drawFullScreen(void* pipeline) = 0;

    /**
     * @brief 绘制程序化网格
     */
    virtual void drawProcedural(void* pipeline, uint32_t vertexCount) = 0;

    /**
     * @brief 执行命令缓冲区
     */
    virtual void executeCommandBuffer(void* cmdBuffer) = 0;

    /**
     * @brief 获取API设备
     */
    virtual void* getAPIDevice() = 0;

    /**
     * @brief 获取API渲染通道
     */
    virtual void* getAPIRenderPass() = 0;
};

/**
 * @brief 可编程渲染特性
 *
 * 用于创建和管理ScriptableRenderPass
 *
 * 这是用户扩展渲染管线的主要接口
 * 用户可以继承此类实现自定义渲染效果
 *
 * 示例实现:
 * class MyBloomFeature : public ScriptableRenderFeature {
 * public:
 *     void Create() override {
 *         myPass_ = new MyBloomPass();
 *         myPass_->renderPassEvent = RenderPassEvent::AfterRendering;
 *     }
 *
 *     void AddRenderPasses(ScriptableRenderer& renderer) override {
 *         renderer.AddRenderPass(myPass_);
 *     }
 *
 * private:
 *     MyBloomPass* myPass_;
 * };
 */
class ScriptableRenderFeature {
public:
    explicit ScriptableRenderFeature(const char* name = "Unnamed Feature");
    virtual ~ScriptableRenderFeature();

    // ========================================================================
    // 生命周期回调
    // ========================================================================

    /**
     * @brief 创建资源
     *
     * 在渲染管线初始化时调用
     * 在这里创建Pass和资源
     */
    virtual void create() {
        // 默认空实现
    }

    /**
     * @brief 添加渲染Pass到管线
     *
     * 在渲染每帧时调用
     * 在这里将创建的Pass添加到渲染器
     *
     * @param renderer 渲染器
     */
    virtual void addRenderPasses(ScriptableRenderer& renderer) = 0;

    /**
     * @brief 销毁资源
     *
     * 在渲染管线销毁时调用
     */
    virtual void destroy() {
        // 默认空实现
    }

    // ========================================================================
    // 配置
    // ========================================================================

    /**
     * @brief 设置是否激活
     */
    void setActive(bool active) { isActive_ = active; }

    /**
     * @brief 是否激活
     */
    bool isActive() const { return isActive_; }

    /**
     * @brief 获取名称
     */
    const char* getName() const { return name_; }

    // ========================================================================
    // Pass管理
    // ========================================================================

    /**
     * @brief 添加Pass
     */
    void addPass(std::unique_ptr<ScriptableRenderPass> pass);

    /**
     * @brief 获取所有Pass
     */
    const std::vector<std::unique_ptr<ScriptableRenderPass>>& getPasses() const {
        return passes_;
    }

    /**
     * @brief 清空所有Pass
     */
    void clearPasses() { passes_.clear(); }

protected:
    const char* name_;
    bool isActive_ = true;
    std::vector<std::unique_ptr<ScriptableRenderPass>> passes_;
};

/**
 * @brief 渲染特性管理器
 *
 * 管理所有ScriptableRenderFeature
 */
class RenderFeatureManager {
public:
    RenderFeatureManager() = default;
    ~RenderFeatureManager() = default;

    // ========================================================================
    // 特性管理
    // ========================================================================

    /**
     * @brief 添加渲染特性
     *
     * @param feature 渲染特性（转移所有权）
     */
    void addFeature(std::unique_ptr<ScriptableRenderFeature> feature);

    /**
     * @brief 移除特性
     *
     * @param name 特性名称
     */
    void removeFeature(const char* name);

    /**
     * @brief 获取特性
     *
     * @param name 特性名称
     * @return 特性指针，如果不存在则返回nullptr
     */
    ScriptableRenderFeature* getFeature(const char* name);

    /**
     * @brief 获取所有特性
     */
    const std::vector<std::unique_ptr<ScriptableRenderFeature>>& getFeatures() const {
        return features_;
    }

    /**
     * @brief 清空所有特性
     */
    void clear() { features_.clear(); }

    // ========================================================================
    // 生命周期
    // ========================================================================

    /**
     * @brief 初始化所有特性
     */
    void initializeAll();

    /**
     * @brief 销毁所有特性
     */
    void destroyAll();

private:
    std::vector<std::unique_ptr<ScriptableRenderFeature>> features_;
};

// ============================================================================
// 预定义的渲染特性（作为示例）
// ============================================================================

/**
 * @brief 后处理渲染特性基类
 *
 * 所有后处理效果的基类
 */
class PostProcessFeature : public ScriptableRenderFeature {
public:
    explicit PostProcessFeature(const char* name);
    ~PostProcessFeature() override = default;

    /**
     * @brief 设置后处理插入点
     */
    void setInjectionPoint(RenderPassEvent evt);

    /**
     * @brief 设置是否启用
     */
    void setEnabled(bool enabled) { enabled_ = enabled; }

    /**
     * @brief 是否启用
     */
    bool isEnabled() const { return enabled_; }

    /**
     * @brief 设置混合模式
     *
     * 控制后处理效果与原始图像的混合方式
     */
    void setBlendMode(BlendMode mode);

protected:
    bool enabled_ = true;
    RenderPassEvent injectionPoint_ = RenderPassEvent::BeforeRenderingPostProcessing;

    enum class BlendMode {
        /** 直接替换原始图像 */
        Replace,

        /** 叠加到原始图像 */
        Add,

        /** 混合原始图像 */
        Blend
    };
    BlendMode blendMode_ = BlendMode::Replace;
};

/**
 * @brief Bloom后处理特性
 *
 * 泛光效果
 */
class BloomFeature : public PostProcessFeature {
public:
    BloomFeature();
    ~BloomFeature() override = default;

    void create() override;
    void addRenderPasses(ScriptableRenderer& renderer) override;

    /**
     * @brief 设置Bloom强度
     */
    void setIntensity(float intensity) { intensity_ = intensity; }

    /**
     * @brief 设置Bloom阈值
     */
    void setThreshold(float threshold) { threshold_ = threshold; }

    /**
     * @brief 设置Bloom迭代次数
     */
    void setIterations(int iterations) { iterations_ = iterations; }

private:
    class BloomPass* bloomPass_ = nullptr;
    float intensity_ = 1.0f;
    float threshold_ = 1.0f;
    int iterations_ = 4;
};

/**
 * @brief 调试渲染特性
 *
 * 用于调试可视化
 */
class DebugRenderFeature : public ScriptableRenderFeature {
public:
    enum class DebugMode {
        None,
        Depth,
        Normal,
        Wireframe,
        UV,
        Albedo,
        Specular,
        Roughness,
        Metallic
    };

    DebugRenderFeature();
    ~DebugRenderFeature() override = default;

    void create() override;
    void addRenderPasses(ScriptableRenderer& renderer) override;

    void setDebugMode(DebugMode mode) { debugMode_ = mode; }
    DebugMode getDebugMode() const { return debugMode_; }

private:
    class DebugPass* debugPass_ = nullptr;
    DebugMode debugMode_ = DebugMode::None;
};
