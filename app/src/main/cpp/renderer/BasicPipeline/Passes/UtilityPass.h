/**
 * @file UtilityPass.h
 * @brief 工具Pass（Blit、Copy、Clear等）
 *
 * 常用工具渲染Pass:
 * - BlitPass: 纹理拷贝（可缩放）
 * - ClearPass: 清空渲染目标
 * - ResolvePass: MSAA解析
 * - GenerateMipsPass: 生成Mip链
 */

#pragma once

#include "../../RenderPass.h"
#include <memory>

// ============================================================================
// Blit Pass
// ============================================================================

/**
 * @brief Blit Pass
 *
 * 从一个纹理拷贝到另一个，支持缩放
 */
class BlitPass : public RenderPass {
public:
    BlitPass();
    ~BlitPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setSource(void* texture, int32_t width, int32_t height) {
        sourceTexture_ = texture;
        sourceWidth_ = width;
        sourceHeight_ = height;
    }

    void setDest(void* texture, int32_t width, int32_t height) {
        destTexture_ = texture;
        destWidth_ = width;
        destHeight_ = height;
    }

    /** 滤波模式 */
    enum class Filter {
        Point,      // 最近邻
        Linear      // 双线性
    };
    void setFilter(Filter filter) { filter_ = filter; }

private:
    void* sourceTexture_ = nullptr;
    int32_t sourceWidth_ = 0;
    int32_t sourceHeight_ = 0;

    void* destTexture_ = nullptr;
    int32_t destWidth_ = 0;
    int32_t destHeight_ = 0;

    Filter filter_ = Filter::Linear;
};

// ============================================================================
// Clear Pass
// ============================================================================

/**
 * @brief Clear Pass
 *
 * 清空渲染目标
 */
class ClearPass : public RenderPass {
public:
    ClearPass();
    ~ClearPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setRenderTarget(void* renderTarget) { renderTarget_ = renderTarget; }

    void setClearColor(float4 color) { clearColor_ = color; }
    void setClearDepth(float depth) { clearDepth_ = depth; }
    void setClearStencil(uint32_t stencil) { clearStencil_ = stencil; }

    /** 清空标志 */
    enum class ClearFlag {
        Color = 1 << 0,
        Depth = 1 << 1,
        Stencil = 1 << 2,
        All = Color | Depth | Stencil
    };
    void setClearFlags(ClearFlag flags) { clearFlags_ = flags; }

private:
    void* renderTarget_ = nullptr;
    float4 clearColor_ = {0, 0, 0, 1};
    float clearDepth_ = 1.0f;
    uint32_t clearStencil_ = 0;
    ClearFlag clearFlags_ = ClearFlag::All;
};

// ============================================================================
// Resolve Pass
// ============================================================================

/**
 * @brief MSAA Resolve Pass
 *
 * 将多重采样渲染目标解析到单采样纹理
 */
class ResolvePass : public RenderPass {
public:
    ResolvePass();
    ~ResolvePass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setSource(void* msaaTexture) { sourceTexture_ = msaaTexture; }
    void setDest(void* resolvedTexture) { destTexture_ = resolvedTexture; }

    /** MSAA样本数 */
    void setSampleCount(uint32_t samples) { sampleCount_ = samples; }

private:
    void* sourceTexture_ = nullptr;
    void* destTexture_ = nullptr;
    uint32_t sampleCount_ = 4;
};

// ============================================================================
// Generate Mips Pass
// ============================================================================

/**
 * @brief Generate Mips Pass
 *
 * 生成纹理的Mip链
 */
class GenerateMipsPass : public RenderPass {
public:
    GenerateMipsPass();
    ~GenerateMipsPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setTexture(void* texture, uint32_t width, uint32_t height, uint32_t mipLevels) {
        texture_ = texture;
        width_ = width;
        height_ = height;
        mipLevels_ = mipLevels;
    }

private:
    void* texture_ = nullptr;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    uint32_t mipLevels_ = 1;
};

// ============================================================================
// Copy Texture Pass
// ============================================================================

/**
 * @brief Copy Texture Pass
 *
 * 纹理1:1拷贝（不缩放）
 */
class CopyTexturePass : public RenderPass {
public:
    CopyTexturePass();
    ~CopyTexturePass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setSource(void* texture) { sourceTexture_ = texture; }
    void setDest(void* texture) { destTexture_ = texture; }

private:
    void* sourceTexture_ = nullptr;
    void* destTexture_ = nullptr;
};

// ============================================================================
// Downsample / Upsample Pass
// ============================================================================

/**
 * @brief Downsample Pass
 *
 * 降采样纹理（用于Bloom等）
 */
class DownsamplePass : public RenderPass {
public:
    DownsamplePass();
    ~DownsamplePass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setSource(void* texture) { sourceTexture_ = texture; }
    void setDest(void* texture) { destTexture_ = texture; }

    void setScale(float scale) { scale_ = scale; }  // 例如: 0.5表示一半大小

private:
    void* sourceTexture_ = nullptr;
    void* destTexture_ = nullptr;
    float scale_ = 0.5f;
};

/**
 * @brief Upsample Pass
 *
 * 升采样纹理
 */
class UpsamplePass : public RenderPass {
public:
    UpsamplePass();
    ~UpsamplePass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setSource(void* texture) { sourceTexture_ = texture; }
    void setDest(void* texture) { destTexture_ = texture; }

    void setScale(float scale) { scale_ = scale; }

private:
    void* sourceTexture_ = nullptr;
    void* destTexture_ = nullptr;
    float scale_ = 2.0f;
};

// ============================================================================
// Gaussian Blur Pass
// ============================================================================

/**
 * @brief 高斯模糊Pass
 *
 * 分离的高斯模糊（水平和垂直分别执行）
 */
class GaussianBlurPass : public RenderPass {
public:
    GaussianBlurPass();
    ~GaussianBlurPass() override = default;

    void initialize(VkDevice device, VkRenderPass renderPass) override;
    void record(VkCommandBuffer cmdBuffer) override;
    void cleanup(VkDevice device) override;

    void setSource(void* texture) { sourceTexture_ = texture; }
    void setOutput(void* texture) { outputTexture_ = texture; }

    void setSigma(float sigma) { sigma_ = sigma; }
    void setKernelSize(int size) { kernelSize_ = size; }

private:
    void* sourceTexture_ = nullptr;
    void* outputTexture_ = nullptr;
    void* tempTexture_ = nullptr;  // 中间纹理

    float sigma_ = 1.0f;
    int kernelSize_ = 5;

    bool firstPass_ = true;  // 标记是否是第一个Pass（水平）
};
