/**
 * @file RenderHandle.cpp
 * @brief 渲染句柄实现
 *
 * 包含纹理池、缓冲池、临时资源池的实现
 */

#include "RenderHandle.h"

#include <vector>
#include <deque>
#include <cstring>

// ============================================================================
// ResourcePool 模板方法实现
// ============================================================================

template<typename T>
ResourcePool<T>::ResourcePool(const PoolConfig& config)
    : config_(config) {
    slots_.reserve(config.initialCapacity);
    freeList_.reserve(config.initialCapacity);
}

template<typename T>
std::pair<uint32_t, typename ResourcePool<T>::Slot*> ResourcePool<T>::Allocate(const char* name) {
    uint32_t index;

    // 优先从FreeList分配
    if (!freeList_.empty()) {
        index = freeList_.back();
        freeList_.pop_back();
    } else {
        // 扩容
        index = static_cast<uint32_t>(slots_.size());
        if (index >= config_.maxCapacity) {
            return {InvalidValue, nullptr};
        }
        slots_.emplace_back();
    }

    auto& slot = slots_[index];
    slot.state = SlotState::Active;
    slot.generation++;
    slot.lastUsedFrame = currentFrame_;

    return {index, &slot};
}

template<typename T>
void ResourcePool<T>::Release(uint32_t index) {
    if (index >= slots_.size()) return;

    auto& slot = slots_[index];
    if (slot.state != SlotState::Active) return;

    if (config_.enableDefragmentation) {
        // 延迟释放：标记为Pending，等待GC统一处理
        slot.state = SlotState::Pending;
    } else {
        // 立即释放
        slot.state = SlotState::Free;
        freeList_.push_back(index);
    }
}

template<typename T>
typename ResourcePool<T>::Slot* ResourcePool<T>::Get(uint32_t index, uint32_t generation) {
    if (index >= slots_.size()) return nullptr;
    auto& slot = slots_[index];
    if (slot.generation != generation) return nullptr;
    if (slot.state != SlotState::Active) return nullptr;
    return &slot;
}

template<typename T>
bool ResourcePool<T>::IsValid(uint32_t index, uint32_t generation) const {
    if (index >= slots_.size()) return false;
    const auto& slot = slots_[index];
    return slot.generation == generation && slot.state == SlotState::Active;
}

template<typename T>
uint32_t ResourcePool<T>::GetActiveCount() const {
    uint32_t count = 0;
    for (const auto& slot : slots_) {
        if (slot.state == SlotState::Active) count++;
    }
    return count;
}

template<typename T>
uint32_t ResourcePool<T>::GetFreeCount() const {
    return static_cast<uint32_t>(freeList_.size());
}

template<typename T>
void ResourcePool<T>::SetCurrentFrame(uint32_t frame) {
    currentFrame_ = frame;
}

template<typename T>
void ResourcePool<T>::GarbageCollect() {
    for (uint32_t i = 0; i < slots_.size(); ++i) {
        if (slots_[i].state == SlotState::Pending) {
            slots_[i].state = SlotState::Free;
            freeList_.push_back(i);
        }
    }
}

template<typename T>
void ResourcePool<T>::Defragment() {
    // 记录旧索引到新索引的映射
    std::vector<uint32_t> remap;
    remap.resize(slots_.size());

    std::vector<Slot> compacted;
    compacted.reserve(slots_.size());

    uint32_t writeIndex = 0;

    // 收集Active资源
    for (uint32_t oldIndex = 0; oldIndex < slots_.size(); ++oldIndex) {
        if (slots_[oldIndex].state == SlotState::Active) {
            remap[oldIndex] = writeIndex;
            compacted.push_back(slots_[oldIndex]);
            writeIndex++;
        }
    }

    // 重置FreeList
    freeList_.clear();
    for (uint32_t i = writeIndex; i < slots_.size(); ++i) {
        freeList_.push_back(i);
    }

    slots_ = std::move(compacted);

    // TODO: 更新外部对旧索引的引用（需要通知机制）
}

template<typename T>
PoolStats ResourcePool<T>::GetStats() const {
    PoolStats stats;
    stats.totalSlots = static_cast<uint32_t>(slots_.size());
    for (const auto& slot : slots_) {
        switch (slot.state) {
            case SlotState::Active:   stats.activeSlots++; break;
            case SlotState::Free:     stats.freeSlots++; break;
            case SlotState::Pending:  stats.pendingSlots++; break;
        }
    }
    return stats;
}

// 显式实例化常用类型
template class ResourcePool<TextureResource>;
template class ResourcePool<BufferResource>;

// ============================================================================
// 纹理池实现
// ============================================================================

TexturePool::TexturePool(const PoolConfig& config)
    : pool_(config) {}

std::pair<TextureHandle, void*> TexturePool::Create(const TextureDesc& desc) {
    auto [index, slot] = pool_.Allocate(desc.name);

    if (index == TextureHandle::InvalidValue) {
        return {TextureHandle(), nullptr};
    }

    // TODO: 调用API创建纹理
    // slot->resource.apiHandle = device->CreateTexture(desc);
    // slot->resource.desc = desc;

    return {
        TextureHandle(index, slot->generation),
        slot->resource.apiHandle
    };
}

void TexturePool::Destroy(TextureHandle handle) {
    auto* slot = pool_.Get(handle.GetIndex(), handle.GetGeneration());
    if (!slot) return;

    // TODO: 调用API销毁纹理
    // device->DestroyTexture(slot->resource.apiHandle);

    pool_.Release(handle.GetIndex());
}

void* TexturePool::GetAPIHandle(TextureHandle handle) {
    auto* slot = pool_.Get(handle.GetIndex(), handle.GetGeneration());
    return slot ? slot->resource.apiHandle : nullptr;
}

bool TexturePool::IsValid(TextureHandle handle) const {
    return pool_.IsValid(handle.GetIndex(), handle.GetGeneration());
}

PoolStats TexturePool::GetStats() const {
    return pool_.GetStats();
}

// ============================================================================
// 缓冲池实现
// ============================================================================

BufferPool::BufferPool(const PoolConfig& config)
    : pool_(config) {}

std::pair<BufferHandle, void*> BufferPool::Create(const BufferDesc& desc) {
    auto [index, slot] = pool_.Allocate(desc.name);

    if (index == BufferHandle::InvalidValue) {
        return {BufferHandle(), nullptr};
    }

    // TODO: 调用API创建缓冲
    // slot->resource.apiHandle = device->CreateBuffer(desc);
    // slot->resource.desc = desc;

    return {
        BufferHandle(index, slot->generation),
        slot->resource.apiHandle
    };
}

void BufferPool::Destroy(BufferHandle handle) {
    auto* slot = pool_.Get(handle.GetIndex(), handle.GetGeneration());
    if (!slot) return;

    // TODO: 调用API销毁缓冲
    // device->DestroyBuffer(slot->resource.apiHandle);

    pool_.Release(handle.GetIndex());
}

void* BufferPool::GetAPIHandle(BufferHandle handle) {
    auto* slot = pool_.Get(handle.GetIndex(), handle.GetGeneration());
    return slot ? slot->resource.apiHandle : nullptr;
}

void* BufferPool::Map(BufferHandle handle) {
    auto* slot = pool_.Get(handle.GetIndex(), handle.GetGeneration());
    if (!slot) return nullptr;

    // TODO: 调用API映射
    // return device->MapBuffer(slot->resource.apiHandle);
    return nullptr;
}

void BufferPool::Unmap(BufferHandle handle) {
    auto* slot = pool_.Get(handle.GetIndex(), handle.GetGeneration());
    if (!slot) return;

    // TODO: 调用API取消映射
    // device->UnmapBuffer(slot->resource.apiHandle);
}

bool BufferPool::IsValid(BufferHandle handle) const {
    return pool_.IsValid(handle.GetIndex(), handle.GetGeneration());
}

PoolStats BufferPool::GetStats() const {
    return pool_.GetStats();
}

// ============================================================================
// 临时纹理池实现
// ============================================================================

TempTexturePool::TempTexturePool() {
    // 预分配槽位
    for (uint32_t i = 0; i < PoolSize; ++i) {
        freeList_.push_back(i);
        entries_.emplace_back();
    }
}

TextureHandle TempTexturePool::Allocate(const TextureDesc& desc) {
    if (freeList_.empty()) return TextureHandle();

    uint32_t index = freeList_.back();
    freeList_.pop_back();

    auto& entry = entries_[index];
    entry.inUse = true;
    entry.generation++;
    entry.desc = desc;

    // TODO: 创建纹理
    // entry.handle = device->CreateTexture(desc);

    return TextureHandle(index, entry.generation);
}

void TempTexturePool::Release(TextureHandle handle) {
    uint32_t index = handle.GetIndex();
    if (index >= PoolSize) return;

    auto& entry = entries_[index];
    if (entry.generation != handle.GetGeneration()) return;

    entry.inUse = false;
    freeList_.push_back(index);
}

void TempTexturePool::Reset() {
    for (uint32_t i = 0; i < PoolSize; ++i) {
        entries_[i].inUse = false;
        entries_[i].generation = 0;
    }
    freeList_.clear();
    for (uint32_t i = 0; i < PoolSize; ++i) {
        freeList_.push_back(i);
    }
}

void* TempTexturePool::Get(TextureHandle handle) {
    uint32_t index = handle.GetIndex();
    if (index >= PoolSize) return nullptr;

    auto& entry = entries_[index];
    if (entry.generation != handle.GetGeneration() || !entry.inUse) {
        return nullptr;
    }

    return entry.handle;
}
