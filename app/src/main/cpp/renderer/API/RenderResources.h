#ifndef PRISMA_ANDROID_RENDER_RESOURCES_H
#define PRISMA_ANDROID_RENDER_RESOURCES_H

#include "API/RenderConfig.h"
#include <vector>

/**
 * @file RenderResources.h
 * @brief 平台无关的渲染资源数据结构
 *
 * 这些结构体存储渲染所需的资源句柄
 * 通过 Native* 类型与具体 API 交互
 */

// ============================================================================
// 渲染对象数据（用于 OpaquePass）
// ============================================================================

/**
 * @brief 渲染对象资源
 *
 * 包含渲染一个对象所需的所有资源
 */
struct RenderObjectResources {
    NativeBuffer vertexBuffer = nullptr;
    NativeBuffer vertexBufferMemory = nullptr;
    NativeBuffer indexBuffer = nullptr;
    NativeBuffer indexBufferMemory = nullptr;

    std::vector<NativeBuffer> uniformBuffers;
    std::vector<NativeBuffer> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;
    std::vector<RenderDescriptorLayout> descriptorSets;
};

// ============================================================================
// Skybox 渲染数据（用于 BackgroundPass）
// ============================================================================

/**
 * @brief Skybox 统一缓冲对象
 */
struct SkyboxUniformData {
    alignas(16) Matrix4 view;
    alignas(16) Matrix4 proj;
};

/**
 * @brief Skybox 渲染资源
 */
struct SkyboxRenderResources {
    NativeBuffer vertexBuffer = nullptr;
    NativeBuffer vertexBufferMemory = nullptr;
    NativeBuffer indexBuffer = nullptr;
    NativeBuffer indexBufferMemory = nullptr;
    NativePipeline pipeline = nullptr;
    NativePipelineLayout pipelineLayout = nullptr;
    RenderDescriptorLayout descriptorSetLayout = nullptr;

    std::vector<RenderDescriptorLayout> descriptorSets;
    std::vector<NativeBuffer> uniformBuffers;
    std::vector<NativeBuffer> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;
    bool hasTexture = false;
};

// ============================================================================
// 纯色渲染数据（用于 BackgroundPass）
// ============================================================================

/**
 * @brief 纯色渲染资源
 */
struct ClearColorResources {
    NativePipeline pipeline = nullptr;
    NativePipelineLayout pipelineLayout = nullptr;
    NativeBuffer vertexBuffer = nullptr;
    NativeBuffer vertexBufferMemory = nullptr;
};

#endif //PRISMA_ANDROID_RENDER_RESOURCES_H