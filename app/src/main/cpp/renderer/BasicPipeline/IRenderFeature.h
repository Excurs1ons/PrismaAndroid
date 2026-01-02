/**
 * @file IRenderFeature.h
 * @brief 渲染特性接口
 *
 * 可扩展的渲染特性系统，类似Unity URP的ScriptableRenderFeature
 * 所有额外的渲染效果都通过实现此接口来添加
 *
 * 设计原则:
 * - 核心 Pass 只负责基础渲染
 * - 所有扩展功能通过 IRenderFeature 实现
 * - Feature 可以在任意渲染阶段插入自己的Pass
 */

#pragma once

#include "RenderHandle.h"
#include "../RenderingData.h"
#include <string>
#include <memory>

// 前向声明
class BasicRenderer;

/**
 * @brief 渲染Pass事件（插入点）
 */
enum class RenderPassEvent {
    BeforeRendering,            // 渲染开始前
    AfterRendering,             // 渲染结束后

    BeforeRenderingShadows,     // 阴影渲染前
    AfterRenderingShadows,      // 阴影渲染后

    BeforeRenderingOpaques,     // 不透明物体前
    AfterRenderingOpaques,      // 不透明物体后

    BeforeRenderingSkybox,      // 天空盒前
    AfterRenderingSkybox,       // 天空盒后

    BeforeRenderingTransparents, // 透明物体前
    AfterRenderingTransparents,  // 透明物体后
};

/**
 * @brief 渲染上下文
 *
 * 提供Feature执行所需的接口（类型安全版本）
 */
class IRenderContext {
public:
    virtual ~IRenderContext() = default;

    /** 获取命令缓冲区 */
    virtual void* GetCommandBuffer() = 0;

    /** 获取API设备 */
    virtual void* GetAPIDevice() = 0;

    /** 获取/创建渲染目标 */
    virtual TextureHandle GetCameraColor() = 0;
    virtual TextureHandle GetCameraDepth() = 0;

    /**
     * @brief 创建临时纹理
     * @return 纹理句柄
     */
    virtual TextureHandle CreateTemporaryTexture(const TextureDesc& desc) = 0;

    /**
     * @brief 释放临时纹理
     */
    virtual void ReleaseTemporaryTexture(TextureHandle handle) = 0;

    /**
     * @brief 创建临时缓冲区
     */
    virtual BufferHandle CreateTemporaryBuffer(const BufferDesc& desc) = 0;

    /**
     * @brief 释放临时缓冲区
     */
    virtual void ReleaseTemporaryBuffer(BufferHandle handle) = 0;

    /** 绘制辅助 */
    virtual void DrawFullScreen(PipelineHandle pipeline) = 0;
    virtual void DrawProcedural(PipelineHandle pipeline, uint32_t vertexCount) = 0;

    /**
     * @brief 获取渲染目标尺寸
     */
    virtual void GetRenderTargetSize(uint32_t& width, uint32_t& height) = 0;

    /**
     * @brief 资源管理器访问（用于转换句柄）
     */
    virtual IResourceManager* GetResourceManager() = 0;
};

/**
 * @brief 渲染特性接口
 *
 * 所有自定义渲染效果都实现此接口
 */
class IRenderFeature {
public:
    explicit IRenderFeature(const char* name) : name_(name) {}
    virtual ~IRenderFeature() = default;

    // ========================================================================
    // 生命周期
    // ========================================================================

    /**
     * @brief 初始化Feature
     * @param context 渲染上下文
     * @return true表示成功
     */
    virtual bool Initialize(IRenderContext& context) { return true; }

    /**
     * @brief 每帧开始时调用
     */
    virtual void OnFrameBegin() {}

    /**
     * @brief 每帧结束时调用
     */
    virtual void OnFrameEnd() {}

    /**
     * @brief 清理资源
     */
    virtual void Cleanup() {}

    // ========================================================================
    // 渲染
    // ========================================================================

    /**
     * @brief 添加渲染Pass到管线
     *
     * 在这里告诉渲染器在哪个阶段执行此Feature
     *
     * @param renderer 渲染器
     */
    virtual void AddRenderPasses(class BasicRenderer& renderer) = 0;

    /**
     * @brief 渲染Feature
     *
     * @param context 渲染上下文
     * @param renderingData 渲染数据
     */
    virtual void Execute(IRenderContext& context, const RenderingData& renderingData) = 0;

    // ========================================================================
    // 配置
    // ========================================================================

    /**
     * @brief 设置是否激活
     */
    void SetActive(bool active) { isActive_ = active; }

    /**
     * @brief 是否激活
     */
    bool IsActive() const { return isActive_; }

    /**
     * @brief 获取名称
     */
    const char* GetName() const { return name_; }

    /**
     * @brief 设置渲染顺序
     * 同一Event下的Feature按此顺序执行
     */
    void SetOrder(int order) { order_ = order; }
    int GetOrder() const { return order_; }

    /**
     * @brief 设置插入点
     */
    void SetPassEvent(RenderPassEvent evt) { passEvent_ = evt; }
    RenderPassEvent GetPassEvent() const { return passEvent_; }

protected:
    const char* name_;
    bool isActive_ = true;
    int order_ = 0;
    RenderPassEvent passEvent_ = RenderPassEvent::AfterRenderingOpaques;
};

/**
 * @brief 渲染特性管理器
 */
class RenderFeatureManager {
public:
    RenderFeatureManager() = default;
    ~RenderFeatureManager() = default;

    /**
     * @brief 添加Feature（转移所有权）
     */
    void AddFeature(std::unique_ptr<IRenderFeature> feature);

    /**
     * @brief 移除Feature
     */
    void RemoveFeature(const char* name);

    /**
     * @brief 获取Feature
     */
    IRenderFeature* GetFeature(const char* name);

    /**
     * @brief 获取所有Feature
     */
    const std::vector<std::unique_ptr<IRenderFeature>>& GetAllFeatures() const {
        return features_;
    }

    /**
     * @brief 清空所有Feature
     */
    void Clear() { features_.clear(); }

    /**
     * @brief 初始化所有Feature
     */
    void InitializeAll(IRenderContext& context);

    /**
     * @brief 清理所有Feature
     */
    void CleanupAll();

private:
    std::vector<std::unique_ptr<IRenderFeature>> features_;
};

// ============================================================================
// 辅助宏 - 便于创建Feature
// ============================================================================

#define DECLARE_RENDER_FEATURE(Class) \
    public: \
        Class(const char* name); \
        virtual ~Class() override; \
        virtual void AddRenderPasses(BasicRenderer& renderer) override; \
        virtual void Execute(IRenderContext& context, const RenderingData& renderingData) override;

#define IMPLEMENT_RENDER_FEATURE_BASE(Class) \
    Class::Class(const char* name) : IRenderFeature(name) {} \
    Class::~Class() = default;
