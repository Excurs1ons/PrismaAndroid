/**
 * @file RenderHandle.cpp
 * @brief 渲染句柄实现
 */

#include "RenderHandle.h"

#include <unordered_map>

// ============================================================================
// 资源管理器实现
// ============================================================================

/**
 * @brief 资源池
 *
 * 管理纹理和缓冲区的分配
 */
class ResourcePool {
public:
    struct TextureEntry {
        void* ptr = nullptr;
        TextureDesc desc;
        uint32_t generation = 0;
        bool inUse = false;
    };

    struct BufferEntry {
        void* ptr = nullptr;
        BufferDesc desc;
        uint32_t generation = 0;
        bool inUse = false;
    };

    // 分配纹理
    TextureHandle AllocateTexture(const TextureDesc& desc, void* ptr) {
        uint32_t index = AllocateIndex(textures_, desc.name);
        textures_[index].ptr = ptr;
        textures_[index].desc = desc;
        textures_[index].inUse = true;
        return TextureHandle(index, textures_[index].generation);
    }

    // 释放纹理
    void ReleaseTexture(TextureHandle handle) {
        if (!handle.IsValid()) return;
        uint32_t idx = handle.GetIndex();
        if (idx < textures_.size()) {
            textures_[idx].inUse = false;
        }
    }

    // 获取纹理指针
    void* GetTexturePtr(TextureHandle handle) {
        if (!handle.IsValid()) return nullptr;
        uint32_t idx = handle.GetIndex();
        if (idx >= textures_.size()) return nullptr;
        if (textures_[idx].generation != handle.GetGeneration()) return nullptr;
        return textures_[idx].ptr;
    }

    // 检查纹理是否有效
    bool IsTextureValid(TextureHandle handle) {
        if (!handle.IsValid()) return false;
        uint32_t idx = handle.GetIndex();
        if (idx >= textures_.size()) return false;
        return textures_[idx].generation == handle.GetGeneration() && textures_[idx].inUse;
    }

    // 分配缓冲区
    BufferHandle AllocateBuffer(const BufferDesc& desc, void* ptr) {
        uint32_t index = AllocateIndex(buffers_, desc.name);
        buffers_[index].ptr = ptr;
        buffers_[index].desc = desc;
        buffers_[index].inUse = true;
        return BufferHandle(index, buffers_[index].generation);
    }

    // 释放缓冲区
    void ReleaseBuffer(BufferHandle handle) {
        if (!handle.IsValid()) return;
        uint32_t idx = handle.GetIndex();
        if (idx < buffers_.size()) {
            buffers_[idx].inUse = false;
        }
    }

    // 获取缓冲指针
    void* GetBufferPtr(BufferHandle handle) {
        if (!handle.IsValid()) return nullptr;
        uint32_t idx = handle.GetIndex();
        if (idx >= buffers_.size()) return nullptr;
        if (buffers_[idx].generation != handle.GetGeneration()) return nullptr;
        return buffers_[idx].ptr;
    }

    // 检查缓冲区是否有效
    bool IsBufferValid(BufferHandle handle) {
        if (!handle.IsValid()) return false;
        uint32_t idx = handle.GetIndex();
        if (idx >= buffers_.size()) return false;
        return buffers_[idx].generation == handle.GetGeneration() && buffers_[idx].inUse;
    }

private:
    std::vector<TextureEntry> textures_;
    std::vector<BufferEntry> buffers_;

    // 分配或复用索引
    template<typename T>
    uint32_t AllocateIndex(std::vector<T>& container) {
        // 查找空闲槽位
        for (uint32_t i = 0; i < container.size(); ++i) {
            if (!container[i].inUse) {
                container[i].generation++;
                return i;
            }
        }
        // 分配新槽位
        uint32_t index = static_cast<uint32_t>(container.size());
        container.emplace_back();
        return index;
    }
};

// ============================================================================
// 资源管理器实现
// ============================================================================

class ResourceManager : public IResourceManager {
public:
    ResourcePool& GetPool() { return pool_; }

    void* GetTexturePtr(TextureHandle handle) override {
        return pool_.GetTexturePtr(handle);
    }

    void* GetBufferPtr(BufferHandle handle) override {
        return pool_.GetBufferPtr(handle);
    }

    bool IsTextureValid(TextureHandle handle) override {
        return pool_.IsTextureValid(handle);
    }

    bool IsBufferValid(BufferHandle handle) override {
        return pool_.IsBufferValid(handle);
    }

private:
    ResourcePool pool_;
};

// ============================================================================
// 全局资源管理器（或由渲染器持有）
// ============================================================================

// 实际使用时，ResourceManager 应该由 BasicRenderer 持有和管理
// 这里只是展示接口实现

// ============================================================================
// 句柄辅助函数
// ============================================================================

namespace HandleUtils {

// 从整数创建句柄
inline TextureHandle TextureFromUint64(uint64_t value) {
    return TextureHandle(
        static_cast<uint32_t>(value & 0xFFFFFFFF),
        static_cast<uint32_t>(value >> 32)
    );
}

inline BufferHandle BufferFromUint64(uint64_t value) {
    return BufferHandle(
        static_cast<uint32_t>(value & 0xFFFFFFFF),
        static_cast<uint32_t>(value >> 32)
    );
}

} // namespace HandleUtils
