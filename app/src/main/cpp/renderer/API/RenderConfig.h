#ifndef PRISMA_ANDROID_RENDER_CONFIG_H
#define PRISMA_ANDROID_RENDER_CONFIG_H

/**
 * @file RenderConfig.h
 * @brief 渲染 API 配置和平台无关类型定义
 *
 * 通过宏选择渲染 API：
 * - RENDER_API_VULKAN: 使用 Vulkan
 * - RENDER_API_D3D12: 使用 DirectX 12
 * - RENDER_API_METAL: 使用 Metal
 */

// 定义当前使用的渲染 API
#ifndef RENDER_API_VULKAN
#define RENDER_API_VULKAN 1
#endif

// ============================================================================
// 平台无关的句柄类型（使用 void* 隔离具体 API）
// ============================================================================

/**
 * @brief 设备句柄（不透明指针）
 */
using RenderDevice = void*;


/**
 * @brief 渲染管线句柄（不透明指针）
 */
using RenderPipelineHandle = void*;

/**
 * @brief 管线布局句柄（不透明指针）
 */
using RenderPipelineLayout = void*;

/**
 * @brief 渲染通道句柄（不透明指针）
 */
using RenderPassHandle = void*;

/**
 * @brief 缓冲区句柄（不透明指针）
 */
using RenderBuffer = void*;

/**
 * @brief 描述符集布局句柄（不透明指针）
 */
using RenderDescriptorLayout = void*;

// ============================================================================
// 通用常量
// ============================================================================

constexpr RenderDevice RENDER_NULL_HANDLE = nullptr;
constexpr RenderPipelineHandle RENDER_PIPELINE_NULL = nullptr;
constexpr RenderPipelineLayout RENDER_LAYOUT_NULL = nullptr;
constexpr RenderPassHandle RENDER_PASS_NULL = nullptr;
constexpr RenderBuffer RENDER_BUFFER_NULL = nullptr;
constexpr RenderDescriptorLayout RENDER_DESCRIPTOR_LAYOUT_NULL = nullptr;

// ============================================================================
// API 特定包含（通过宏切换）
// ============================================================================

#if RENDER_API_VULKAN
    #include "VulkanConfig.h"
    using NativeDevice = VkDevice;
    using NativeCommandList = VkCommandBuffer;
    using NativePipeline = VkPipeline;
    using NativePipelineLayout = VkPipelineLayout;
    using NativeRenderPass = VkRenderPass;
    using NativeBuffer = VkBuffer;
    using NativeDescriptorLayout = VkDescriptorSetLayout;

    #define RENDER_DEVICE_TO_NATIVE(device) static_cast<VkDevice>(device)
    #define RENDER_NATIVE_TO_DEVICE(native) static_cast<RenderDevice>(native)
    #define RENDER_CMD_TO_NATIVE(cmd) static_cast<VkCommandBuffer>(cmd)
    #define RENDER_NATIVE_TO_CMD(native) static_cast<RenderCommandList>(native)

#elif RENDER_API_D3D12
    #include "D3D12Config.h"
    // TODO: 实现 DirectX 12 类型映射
    #error "DirectX 12 not yet implemented"

#elif RENDER_API_METAL
    #include "MetalConfig.h"
    // TODO: 实现 Metal 类型映射
    #error "Metal not yet implemented"

#else
    #error "No render API selected! Please define RENDER_API_VULKAN, RENDER_API_D3D12, or RENDER_API_METAL"
#endif

// ============================================================================
// 通用数据容器结构体（平台无关）
// ============================================================================

/**
 * @brief 2D 尺寸（用于纹理、视口等）
 */
struct RenderExtent2D {
    uint32_t width;
    uint32_t height;
};

/**
 * @brief 表面变换标志位
 */
enum class RenderSurfaceTransform : uint32_t {
    Identity = 0,
    Rotate90 = 1,
    Rotate180 = 2,
    Rotate270 = 3,
    HorizontalMirror = 4,
    HorizontalMirrorRotate90 = 5,
    VerticalMirror = 6,
    VerticalMirrorRotate90 = 7,
    Inherit = 8
};

#endif //PRISMA_ANDROID_RENDER_CONFIG_H
