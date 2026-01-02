/**
 * @file RenderHandle.h
 * @brief 渲染资源句柄 - 类型安全的资源引用
 *
 * 替代 void*，提供类型安全的资源访问
 */

#pragma once

#include <cstdint>
#include <functional>

// ============================================================================
// 句柄类型定义
// ============================================================================

/**
 * @brief 通用句柄基类
 *
 * 使用索引+世代的设计，避免悬空引用
 */
template<typename Tag, typename IndexType = uint32_t>
class Handle {
public:
    static constexpr IndexType InvalidValue = ~IndexType(0);

    Handle() : index_(InvalidValue), generation_(0) {}

    explicit Handle(IndexType index) : index_(index), generation_(0) {}

    Handle(IndexType index, IndexType generation)
        : index_(index), generation_(generation) {}

    bool IsValid() const { return index_ != InvalidValue; }

    IndexType GetIndex() const { return index_; }
    IndexType GetGeneration() const { return generation_; }

    bool operator==(const Handle& other) const {
        return index_ == other.index_ && generation_ == other.generation_;
    }

    bool operator!=(const Handle& other) const {
        return !(*this == other);
    }

    // 转换为整数（用于哈希等）
    explicit operator uint64_t() const {
        return (uint64_t(generation_) << 32) | index_;
    }

private:
    IndexType index_;
    IndexType generation_;
};

// ============================================================================
// 具体句柄类型
// ============================================================================

struct TextureTag {};
struct BufferTag {};
struct PipelineTag {};
struct RenderPassTag {};
struct FramebufferTag {};
struct ShaderTag {};

using TextureHandle = Handle<TextureTag, uint32_t>;
using BufferHandle = Handle<BufferTag, uint32_t>;
using PipelineHandle = Handle<PipelineTag, uint32_t>;
using RenderPassHandle = Handle<RenderPassTag, uint32_t>;
using FramebufferHandle = Handle<FramebufferTag, uint32_t>;
using ShaderHandle = Handle<ShaderTag, uint32_t>;

// ============================================================================
// 渲染目标描述
// ============================================================================

/**
 * @brief 渲染目标句柄
 */
struct RenderTargetHandle {
    static constexpr uint32_t Invalid = ~0u;
    uint32_t id = Invalid;

    bool IsValid() const { return id != Invalid; }

    // 预定义渲染目标
    static constexpr uint32_t CameraColor = 0;
    static constexpr uint32_t CameraDepth = 1;
    static constexpr uint32_t Temp0 = 2;
    static constexpr uint32_t Temp1 = 3;
    static constexpr uint32_t Temp2 = 4;
    static constexpr uint32_t Temp3 = 5;
    static constexpr uint32_t User0 = 16;  // 用户自定义起始
};

// ============================================================================
// 纹理描述
// ============================================================================

/**
 * @brief 纹理格式
 */
enum class TextureFormat {
    Unknown,

    // 8位格式
    R8,
    RG8,
    RGB8,
    RGBA8,
    SRGB8,
    SRGB8_A8,

    // 16位格式
    R16,
    RG16,
    RGB16,
    RGBA16,
    R16F,
    RG16F,
    RGB16F,
    RGBA16F,

    // 32位格式
    R32F,
    RG32F,
    RGB32F,
    RGBA32F,

    // 深度格式
    Depth16,
    Depth24Stencil8,
    Depth32F,

    // 压缩格式
    BC1,
    BC2,
    BC3,
    BC4,
    BC5,
    BC6H,
    BC7
};

/**
 * @brief 纹理描述
 */
struct TextureDesc {
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t depth = 1;           // 对于纹理数组
    uint32_t mipLevels = 1;
    TextureFormat format = TextureFormat::RGBA8;
    const char* name = "Texture";

    // 创建标志
    bool createRenderTarget = false;
    bool createUAV = false;        // Unordered Access View
    bool allowSampling = true;
};

// ============================================================================
// 缓冲描述
// ============================================================================

/**
 * @brief 缓冲使用方式
 */
enum class BufferUsage {
    TransferSrc,      // 转换源
    TransferDst,      // 转换目标
    Uniform,          // Uniform缓冲
    Storage,          // 存储缓冲
    Index,            // 索引缓冲
    Vertex,           // 顶点缓冲
    Indirect          // 间接绘制
};

/**
 * @brief 缓冲描述
 */
struct BufferDesc {
    uint64_t size = 0;
    BufferUsage usage = BufferUsage::Vertex;
    const char* name = "Buffer";
};

// ============================================================================
// 资源访问器
// ============================================================================

/**
 * @brief 资源管理器接口
 *
 * 提供句柄到实际资源的转换
 */
class IResourceManager {
public:
    virtual ~IResourceManager() = default;

    /**
     * @brief 获取纹理实际指针（内部使用）
     */
    virtual void* GetTexturePtr(TextureHandle handle) = 0;

    /**
     * @brief 获取缓冲实际指针（内部使用）
     */
    virtual void* GetBufferPtr(BufferHandle handle) = 0;

    /**
     * @brief 检查句柄是否有效
     */
    virtual bool IsTextureValid(TextureHandle handle) = 0;
    virtual bool IsBufferValid(BufferHandle handle) = 0;
};

// ============================================================================
// 类型安全的资源引用
// ============================================================================

/**
 * @brief 纹理引用
 *
 * 自动检查类型有效性
 */
class TextureRef {
public:
    TextureRef() : handle_(), manager_(nullptr) {}

    TextureRef(TextureHandle handle, IResourceManager* manager)
        : handle_(handle), manager_(manager) {}

    bool IsValid() const {
        return handle_.IsValid() &&
               (manager_ == nullptr || manager_->IsTextureValid(handle_));
    }

    TextureHandle GetHandle() const { return handle_; }

    // 隐式转换为句柄
    operator TextureHandle() const { return handle_; }

private:
    TextureHandle handle_;
    IResourceManager* manager_;
};

/**
 * @brief 缓冲引用
 */
class BufferRef {
public:
    BufferRef() : handle_(), manager_(nullptr) {}

    BufferRef(BufferHandle handle, IResourceManager* manager)
        : handle_(handle), manager_(manager) {}

    bool IsValid() const {
        return handle_.IsValid() &&
               (manager_ == nullptr || manager_->IsBufferValid(handle_));
    }

    BufferHandle GetHandle() const { return handle_; }

    operator BufferHandle() const { return handle_; }

private:
    BufferHandle handle_;
    IResourceManager* manager_;
};

// ============================================================================
// 类型化的渲染数据
// ============================================================================

/**
 * @brief 渲染目标视图
 */
struct RenderTargetView {
    TextureHandle texture;
    uint32_t mipSlice = 0;
    uint32_t arraySlice = 0;

    bool IsValid() const { return texture.IsValid(); }
};

/**
 * @brief 深度模板视图
 */
struct DepthStencilView {
    TextureHandle texture;
    uint32_t mipSlice = 0;
    uint32_t arraySlice = 0;

    bool IsValid() const { return texture.IsValid(); }
};

/**
 * @brief 渲染目标绑定
 */
struct RenderTargetBinding {
    RenderTargetView color;
    DepthStencilView depth;
    uint32_t width;
    uint32_t height;

    bool IsValid() const {
        return color.IsValid() || depth.IsValid();
    }
};

// ============================================================================
// 采样器描述
// ============================================================================

/**
 * @brief 纹理寻址模式
 */
enum class TextureAddressMode {
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder,
    MirrorClampToEdge
};

/**
 * @brief 过滤模式
 */
enum class TextureFilterMode {
    Point,
    Linear,
    Trilinear,
    Anisotropic
};

/**
 * @brief 采样器描述
 */
struct SamplerDesc {
    TextureFilterMode filter = TextureFilterMode::Linear;
    TextureAddressMode addressU = TextureAddressMode::Repeat;
    TextureAddressMode addressV = TextureAddressMode::Repeat;
    TextureAddressMode addressW = TextureAddressMode::Repeat;
    float mipLodBias = 0.0f;
    float maxAnisotropy = 16.0f;
    const char* name = "Sampler";
};

using SamplerHandle = Handle<SamplerTag, uint32_t>;

// ============================================================================
// Shader资源视图
// ============================================================================

/**
 * @brief Shader资源视图类型
 */
enum class SRVType {
    Texture,
    TextureArray,
    Buffer,
    StructuredBuffer,
    ByteAddressBuffer
};

/**
 * @brief Shader资源视图
 */
struct ShaderResourceView {
    union {
        TextureHandle texture;
        BufferHandle buffer;
    };
    SRVType type;
    uint32_t firstElement = 0;
    uint32_t numElements = 0;
    uint32_t constantOffset = 0;
};

// ============================================================================
// 杂项定义
// ============================================================================

/**
 * @brief 视口
 */
struct Viewport {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float minDepth = 0.0f;
    float maxDepth = 1.0f;
};

/**
 * @brief 裁剪矩形
 */
struct Rect {
    int32_t x = 0;
    int32_t y = 0;
    uint32_t width = 0;
    uint32_t height = 0;
};

/**
 * @brief 清除值
 */
struct ClearValue {
    union {
        struct {
            float r, g, b, a;
        } color;
        struct {
            float depth;
            uint32_t stencil;
        } depthStencil;
    };

    static ClearValue Color(float r, float g, float b, float a = 1.0f) {
        ClearValue v;
        v.color.r = r;
        v.color.g = g;
        v.color.b = b;
        v.color.a = a;
        return v;
    }

    static ClearValue DepthStencil(float depth = 1.0f, uint32_t stencil = 0) {
        ClearValue v;
        v.depthStencil.depth = depth;
        v.depthStencil.stencil = stencil;
        return v;
    }
};

// ============================================================================
// 资源池类型
// ============================================================================

/**
 * @brief 资源槽状态
 */
enum class SlotState : uint8_t {
    Free,     // 空闲，可分配
    Active,   // 使用中
    Pending   // 待释放（延迟释放）
};

/**
 * @brief 资源池配置
 */
struct PoolConfig {
    uint32_t initialCapacity = 64;      // 初始容量
    uint32_t maxCapacity = 4096;        // 最大容量
    bool enableDefragmentation = false; // 启用碎片整理
    bool enableThreadSafe = false;      // 线程安全（加锁）
};

/**
 * @brief 资源池统计
 */
struct PoolStats {
    uint32_t totalSlots = 0;
    uint32_t activeSlots = 0;
    uint32_t freeSlots = 0;
    uint32_t pendingSlots = 0;
};

// ============================================================================
// 资源池（模板）
// ============================================================================

/**
 * @brief 资源池
 *
 * 功能:
 * - 对象复用（FreeList）
 * - 世代计数（防止悬空引用）
 * - 延迟释放（避免频繁分配/释放）
 * - 碎片整理
 */
template<typename T>
class ResourcePool {
public:
    struct Slot {
        T resource;
        SlotState state = SlotState::Free;
        uint32_t generation = 0;
        uint32_t lastUsedFrame = 0;
    };

    explicit ResourcePool(const PoolConfig& config = PoolConfig());
    virtual ~ResourcePool() = default;

    // 禁止拷贝
    ResourcePool(const ResourcePool&) = delete;
    ResourcePool& operator=(const ResourcePool&) = delete;

    /**
     * @brief 分配资源
     * @return (索引, Slot指针)
     */
    std::pair<uint32_t, Slot*> Allocate(const char* name = nullptr);

    /**
     * @brief 释放资源
     */
    void Release(uint32_t index);

    /**
     * @brief 获取资源
     */
    Slot* Get(uint32_t index, uint32_t generation);

    /**
     * @brief 检查资源是否有效
     */
    bool IsValid(uint32_t index, uint32_t generation) const;

    /**
     * @brief 获取使用中的资源数量
     */
    uint32_t GetActiveCount() const;

    /**
     * @brief 获取空闲资源数量
     */
    uint32_t GetFreeCount() const;

    /**
     * @brief 设置当前帧
     */
    void SetCurrentFrame(uint32_t frame);

    /**
     * @brief 垃圾回收
     */
    void GarbageCollect();

    /**
     * @brief 碎片整理
     */
    void Defragment();

    /**
     * @brief 获取统计信息
     */
    PoolStats GetStats() const;

private:
    PoolConfig config_;
    std::vector<Slot> slots_;
    std::deque<uint32_t> freeList_;
    uint32_t currentFrame_ = 0;
};

// ============================================================================
// 纹理资源
// ============================================================================

/**
 * @brief 纹理资源
 */
struct TextureResource {
    void* apiHandle = nullptr;         // API特定的纹理句柄 (VkImage等)
    void* allocationHandle = nullptr;  // 内存分配句柄 (VmaAllocation等)
    void* viewHandle = nullptr;        // 纹理视图句柄 (ImageView)
    TextureDesc desc;
};

/**
 * @brief 纹理池
 */
class TexturePool {
public:
    explicit TexturePool(const PoolConfig& config = PoolConfig());

    /**
     * @brief 创建纹理
     * @return (句柄, API句柄)
     */
    std::pair<TextureHandle, void*> Create(const TextureDesc& desc);

    /**
     * @brief 销毁纹理
     */
    void Destroy(TextureHandle handle);

    /**
     * @brief 获取API句柄
     */
    void* GetAPIHandle(TextureHandle handle);

    /**
     * @brief 检查是否有效
     */
    bool IsValid(TextureHandle handle) const;

    /**
     * @brief 获取统计
     */
    PoolStats GetStats() const;

private:
    ResourcePool<TextureResource> pool_;
};

// ============================================================================
// 缓冲资源
// ============================================================================

/**
 * @brief 缓冲资源
 */
struct BufferResource {
    void* apiHandle = nullptr;         // API缓冲句柄 (VkBuffer等)
    void* allocationHandle = nullptr;  // 内存分配句柄
    void* mappedPtr = nullptr;         // 映射指针（CPU可访问）
    BufferDesc desc;
};

/**
 * @brief 缓冲池
 */
class BufferPool {
public:
    explicit BufferPool(const PoolConfig& config = PoolConfig());

    /**
     * @brief 创建缓冲
     * @return (句柄, API句柄)
     */
    std::pair<BufferHandle, void*> Create(const BufferDesc& desc);

    /**
     * @brief 销毁缓冲
     */
    void Destroy(BufferHandle handle);

    /**
     * @brief 获取API句柄
     */
    void* GetAPIHandle(BufferHandle handle);

    /**
     * @brief 映射缓冲（CPU访问）
     */
    void* Map(BufferHandle handle);

    /**
     * @brief 取消映射
     */
    void Unmap(BufferHandle handle);

    /**
     * @brief 检查是否有效
     */
    bool IsValid(BufferHandle handle) const;

    /**
     * @brief 获取统计
     */
    PoolStats GetStats() const;

private:
    ResourcePool<BufferResource> pool_;
};

// ============================================================================
// 临时资源池
// ============================================================================

/**
 * @brief 临时纹理池
 *
 * 用于帧临时资源，每帧自动回收
 */
class TempTexturePool {
public:
    static constexpr uint32_t PoolSize = 32;

    TempTexturePool();

    /**
     * @brief 分配临时纹理
     */
    TextureHandle Allocate(const TextureDesc& desc);

    /**
     * @brief 释放临时纹理
     */
    void Release(TextureHandle handle);

    /**
     * @brief 每帧重置（释放所有临时纹理）
     */
    void Reset();

    /**
     * @brief 获取纹理句柄
     */
    void* Get(TextureHandle handle);

private:
    struct Entry {
        bool inUse = false;
        uint32_t generation = 0;
        TextureDesc desc;
        void* handle = nullptr;
    };

    std::array<Entry, PoolSize> entries_;
    std::vector<uint32_t> freeList_;
};
