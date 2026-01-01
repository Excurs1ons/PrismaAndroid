/**
 * @file RenderQueue.h
 * @brief 渲染队列 - 管理渲染对象的排序和提交
 *
 * 功能:
 * - 按渲染队列ID分类（不透明、透明等）
 * - 按距离排序（从前到后/从后到前）
 * - 材质排序（减少状态切换）
 * - 批量提交
 */

#pragma once

#include "../MathTypes.h"
#include "../Component.h"
#include "DepthState.h"
#include "StencilState.h"
#include <vector>
#include <memory>
#include <functional>

// 前向声明
class GameObject;
class Material;

/**
 * @brief 渲染队列ID
 *
 * 决定渲染顺序，值越小越先渲染
 * 参考 Unity URP 的渲染队列定义
 */
namespace RenderQueueId {
    enum {
        // ====================================================================
        // 背景队列（最早渲染，无深度测试）
        // ====================================================================
        Background = 1000,

        // ====================================================================
        // 不透明队列（标准深度测试）
        // ====================================================================
        Opaque = 2000,

        /** 不透明但使用 Alpha Test 的物体 */
        AlphaTest = 2450,

        // ====================================================================
        // 透明队列（深度测试但不写入深度）
        // ====================================================================
        Transparent = 3000,

        // ====================================================================
        // 覆盖队列（最后渲染，禁用深度测试）- UI等
        // ====================================================================
        Overlay = 4000
    };
}

/**
 * @brief 排序模式
 */
enum class SortMode {
    /** 不排序（按添加顺序） */
    None = 0,

    /** 从前到后（不透明物体推荐，利用Early-Z） */
    FrontToBack = 1,

    /** 从后到前（透明物体必需） */
    BackToFront = 2,

    /** 按材质分组（减少状态切换） */
    ByMaterial = 3,

    /** 混合模式：先按材质，再按距离 */
    MaterialThenDistance = 4
};

/**
 * @brief 渲染对象数据
 *
 * 包含渲染一个物体所需的所有信息
 */
struct RenderObject {
    // ========================================================================
    // 对象标识
    // ========================================================================

    /** 对应的GameObject */
    GameObject* gameObject = nullptr;

    /** 对象名称（用于调试） */
    const char* name = "Unnamed";

    // ========================================================================
    // 变换
    // ========================================================================

    /** 世界变换矩阵 */
    Matrix4 worldMatrix;

    /** 对象中心点（世界空间，用于距离排序） */
    Vector3 center;

    /** 包围球半径（用于视锥剔除） */
    float radius = 0.0f;

    // ========================================================================
    // 渲染资源
    // ========================================================================

    /** 材质 */
    Material* material = nullptr;

    /** 渲染API相关的几何体句柄 */
    void* geometryHandle = nullptr;

    /** 子网格索引（对于多网格物体） */
    int subMeshIndex = -1;

    // ========================================================================
    // 渲染状态
    // ========================================================================

    /** 渲染队列ID */
    uint32_t queueID = RenderQueueId::Opaque;

    /** 深度状态 */
    DepthState depthState;

    /** 模板状态 */
    StencilState stencilState;

    // ========================================================================
    // 排序键
    // ========================================================================

    /** 到相机的距离（用于排序） */
    float distanceToCamera = 0.0f;

    /** 材质ID（用于材质排序） */
    uint64_t materialID = 0;

    /**
     * @brief 计算排序键
     *
     * 根据排序模式生成排序键，使得std::sort可以正确排序
     *
     * @param sortMode 排序模式
     * @return 排序键（越小越靠前）
     */
    uint64_t calculateSortKey(SortMode sortMode) const {
        switch (sortMode) {
            case SortMode::None:
                return 0;

            case SortMode::FrontToBack: {
                // 距离越小越靠前
                // 使用 IEEE 754 浮点数的位表示作为排序键
                uint32_t distBits;
                std::memcpy(&distBits, &distanceToCamera, sizeof(float));
                // 对于负数（罕见），需要反转位
                if (distBits & 0x80000000) {
                    distBits = ~distBits;
                } else {
                    distBits ^= 0x80000000;  // 反转符号位
                }
                return static_cast<uint64_t>(distBits);
            }

            case SortMode::BackToFront: {
                // 距离越大越靠前
                uint32_t distBits;
                std::memcpy(&distBits, &distanceToCamera, sizeof(float));
                return static_cast<uint64_t>(distBits);
            }

            case SortMode::ByMaterial:
                // 材质ID越小越靠前
                return materialID;

            case SortMode::MaterialThenDistance: {
                // 高32位为材质ID，低32位为距离
                uint32_t distBits;
                std::memcpy(&distBits, &distanceToCamera, sizeof(float));
                return (materialID << 32) | distBits;
            }

            default:
                return 0;
        }
    }

    /**
     * @brief 排序比较函数对象
     */
    struct Comparator {
        SortMode mode;

        bool operator()(const RenderObject& a, const RenderObject& b) const {
            switch (mode) {
                case SortMode::FrontToBack:
                    return a.distanceToCamera < b.distanceToCamera;

                case SortMode::BackToFront:
                    return a.distanceToCamera > b.distanceToCamera;

                case SortMode::ByMaterial:
                    return a.materialID < b.materialID;

                case SortMode::MaterialThenDistance:
                    if (a.materialID != b.materialID) {
                        return a.materialID < b.materialID;
                    }
                    return a.distanceToCamera < b.distanceToCamera;

                case SortMode::None:
                default:
                    return false;
            }
        }
    };
};

/**
 * @brief 渲染队列
 *
 * 管理一组RenderObject，负责排序和剔除
 */
class RenderQueue {
public:
    /**
     * @brief 构造渲染队列
     * @param queueID 队列ID
     * @param name 队列名称
     */
    RenderQueue(uint32_t queueID, const char* name);

    virtual ~RenderQueue() = default;

    // ========================================================================
    // 对象管理
    // ========================================================================

    /**
     * @brief 添加渲染对象
     * @param obj 渲染对象
     */
    void addObject(const RenderObject& obj);

    /**
     * @brief 批量添加渲染对象
     * @param objects 对象列表
     */
    void addObjects(const std::vector<RenderObject>& objects);

    /**
     * @brief 清空队列
     */
    void clear();

    // ========================================================================
    // 排序
    // ========================================================================

    /**
     * @brief 设置排序模式
     * @param mode 排序模式
     */
    void setSortMode(SortMode mode) { sortMode_ = mode; }

    /** @brief 获取排序模式 */
    SortMode getSortMode() const { return sortMode_; }

    /**
     * @brief 对队列进行排序
     */
    void sort();

    // ========================================================================
    // 渲染
    // ========================================================================

    /**
     * @brief 提交队列中的所有对象进行渲染
     * @param commandBuffer 命令缓冲区
     */
    void submit(void* commandBuffer);

    /**
     * @brief 获取队列中的所有对象
     */
    const std::vector<RenderObject>& getObjects() const { return objects_; }

    /**
     * @brief 获取对象数量
     */
    size_t getObjectCount() const { return objects_.size(); }

    // ========================================================================
    // 调试
    // ========================================================================

    /**
     * @brief 获取队列ID
     */
    uint32_t getQueueID() const { return queueID_; }

    /**
     * @brief 获取队列名称
     */
    const char* getName() const { return name_; }

private:
    uint32_t queueID_;
    const char* name_;
    SortMode sortMode_ = SortMode::None;
    std::vector<RenderObject> objects_;
    bool isSorted_ = false;
};

/**
 * @brief 渲染队列管理器
 *
 * 管理所有渲染队列，处理物体分配和渲染
 */
class RenderQueueManager {
public:
    RenderQueueManager();
    ~RenderQueueManager() = default;

    // ========================================================================
    // 队列管理
    // ========================================================================

    /**
     * @brief 添加渲染对象到合适的队列
     *
     * 根据 queueID 自动分配到对应的队列
     *
     * @param obj 渲染对象
     */
    void addObject(const RenderObject& obj);

    /**
     * @brief 创建自定义队列
     * @param queueID 队列ID
     * @param name 队列名称
     * @return 创建的队列指针
     */
    RenderQueue* createQueue(uint32_t queueID, const char* name);

    /**
     * @brief 获取指定ID的队列
     * @param queueID 队列ID
     * @return 队列指针，如果不存在则返回nullptr
     */
    RenderQueue* getQueue(uint32_t queueID);

    /**
     * @brief 清空所有队列
     */
    void clear();

    /**
     * @brief 对所有队列进行排序
     */
    void sortAll();

    /**
     * @brief 按顺序提交所有队列
     * @param commandBuffer 命令缓冲区
     */
    void submitAll(void* commandBuffer);

    // ========================================================================
    // 预定义队列
    // ========================================================================

    /** 背景队列 */
    RenderQueue* getBackgroundQueue() { return backgroundQueue_; }

    /** 不透明队列 */
    RenderQueue* getOpaqueQueue() { return opaqueQueue_; }

    /** Alpha Test队列 */
    RenderQueue* getAlphaTestQueue() { return alphaTestQueue_; }

    /** 透明队列 */
    RenderQueue* getTransparentQueue() { return transparentQueue_; }

    /** 覆盖队列（UI） */
    RenderQueue* getOverlayQueue() { return overlayQueue_; }

    // ========================================================================
    // 调试
    // ========================================================================

    /**
     * @brief 获取统计信息
     */
    struct Stats {
        uint32_t totalObjects = 0;
        uint32_t backgroundObjects = 0;
        uint32_t opaqueObjects = 0;
        uint32_t alphaTestObjects = 0;
        uint32_t transparentObjects = 0;
        uint32_t overlayObjects = 0;
    };
    const Stats& getStats() const { return stats_; }

    void resetStats() { stats_ = Stats(); }

private:
    std::vector<std::unique_ptr<RenderQueue>> queues_;

    // 预定义队列（快速访问）
    RenderQueue* backgroundQueue_ = nullptr;
    RenderQueue* opaqueQueue_ = nullptr;
    RenderQueue* alphaTestQueue_ = nullptr;
    RenderQueue* transparentQueue_ = nullptr;
    RenderQueue* overlayQueue_ = nullptr;

    Stats stats_;
};

/**
 * @brief 渲染队列构建器
 *
 * 从场景中的GameObject构建渲染队列
 */
class RenderQueueBuilder {
public:
    /**
     * @brief 从场景构建渲染队列
     *
     * 步骤:
     * 1. 遍历所有GameObject
     * 2. 查找MeshRenderer组件
     * 3. 计算到相机的距离
     * 4. 添加到对应的队列
     * 5. 进行视锥剔除（可选）
     *
     * @param gameObjects 游戏对象列表
     * @param cameraPosition 相机位置（用于距离计算）
     * @param queueManager 输出的队列管理器
     * @param enableFrustumCulling 是否启用视锥剔除
     */
    static void build(const std::vector<std::shared_ptr<GameObject>>& gameObjects,
                      const Vector3& cameraPosition,
                      RenderQueueManager& queueManager,
                      bool enableFrustumCulling = true);

    /**
     * @brief 从GameObject创建RenderObject
     * @param gameObject 游戏对象
     * @param cameraPosition 相机位置
     * @return 创建的RenderObject，如果无效则返回null
     */
    static std::optional<RenderObject> createFromGameObject(
        GameObject* gameObject,
        const Vector3& cameraPosition);
};
