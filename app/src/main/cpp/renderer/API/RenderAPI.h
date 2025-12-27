#ifndef PRISMA_ANDROID_RENDER_API_H
#define PRISMA_ANDROID_RENDER_API_H

/**
 * @file RenderAPI.h
 * @brief 渲染 API 抽象层接口
 *
 * 定义跨渲染 API 的抽象类型，支持 Vulkan、DirectX 12、Metal 等
 */

namespace RenderAPI {

// ============================================================================
// 前向声明（不包含具体 API 头文件）
// ============================================================================

struct DeviceHandle;
struct CommandListHandle;
struct PipelineHandle;
struct PipelineLayoutHandle;
struct BufferHandle;
struct TextureHandle;
struct DescriptorSetLayoutHandle;
struct DescriptorSetHandle;
struct RenderPassHandle;

// ============================================================================
// 抽象句柄类型
// ============================================================================

/**
 * @brief 设备句柄
 * 对应: Vulkan(VkDevice), D3D12(ID3D12Device), Metal(MTLDevice)
 */
using Device = DeviceHandle*;

/**
 * @brief 命令列表句柄
 * 对应: Vulkan(VkCommandBuffer), D3D12(ID3D12GraphicsCommandList), Metal(MTLCommandEncoder)
 */
using CommandList = CommandListHandle*;

/**
 * @brief 渲染管线句柄
 * 对应: Vulkan(VkPipeline), D3D12(ID3D12PipelineState), Metal(MTLRenderPipelineState)
 */
using Pipeline = PipelineHandle*;

/**
 * @brief 管线布局句柄
 * 对应: Vulkan(VkPipelineLayout), D3D12(Root Signature), Metal(MTLArgumentEncoder)
 */
using PipelineLayout = PipelineLayoutHandle*;

/**
 * @brief 缓冲区句柄
 * 对应: Vulkan(VkBuffer), D3D12(ID3D12Resource), Metal(MTLBuffer)
 */
using Buffer = BufferHandle*;

/**
 * @brief 纹理句柄
 * 对应: Vulkan(VkImage), D3D12(ID3D12Resource), Metal(MTLTexture)
 */
using Texture = TextureHandle*;

/**
 * @brief 描述符集布局句柄
 * 对应: Vulkan(VkDescriptorSetLayout), D3D12(Root Signature), Metal(Argument Buffer)
 */
using DescriptorSetLayout = DescriptorSetLayoutHandle*;

/**
 * @brief 描述符集句柄
 * 对应: Vulkan(VkDescriptorSet), D3D12(Descriptor Heap), Metal(Argument Buffer)
 */
using DescriptorSet = DescriptorSetHandle*;

/**
 * @brief 渲染通道句柄
 * 对应: Vulkan(VkRenderPass), D3D12(无直接对应), Metal(MTLRenderPass)
 */
using RenderPass = RenderPassHandle*;

// ============================================================================
// 常量
// ============================================================================

constexpr nullptr_t NULL_HANDLE = nullptr;

} // namespace RenderAPI

#endif //PRISMA_ANDROID_RENDER_API_H
