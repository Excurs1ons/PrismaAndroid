/**
 * @file Frustum.h
 * @brief 视锥体 - 用于视锥剔除（Frustum Culling）
 *
 * 视锥体由6个平面组成，用于判断物体是否在相机视野内
 */

#pragma once

#include "../../MathTypes.h"

/**
 * @brief 平面方程: ax + by + cz + d = 0
 *
 * 平面由法向量 (a, b, c) 和距离 d 定义
 * 法向量指向平面外侧（即指向可见区域）
 */
struct Plane {
    float a, b, c, d;  // 平面方程系数

    Plane() : a(0), b(0), c(0), d(0) {}
    Plane(float _a, float _b, float _c, float _d) : a(_a), b(_b), c(_c), d(_d) {}

    /**
     * @brief 从法向量和距离构建平面
     * @param normal 归一化的法向量
     * @param distance 从原点到平面的有向距离
     */
    static Plane fromNormalDistance(const Vector3& normal, float distance) {
        return Plane(normal.x, normal.y, normal.z, distance);
    }

    /**
     * @brief 从点和法向量构建平面
     * @param point 平面上的一点
     * @param normal 归一化的法向量
     */
    static Plane fromPointNormal(const Vector3& point, const Vector3& normal) {
        float d = -glm::dot(normal, point);
        return Plane(normal.x, normal.y, normal.z, d);
    }

    /**
     * @brief 获取法向量
     */
    Vector3 getNormal() const {
        return Vector3(a, b, c);
    }

    /**
     * @brief 归一化平面（确保法向量是单位向量）
     */
    void normalize() {
        float length = std::sqrt(a * a + b * b + c * c);
        if (length > 0.0001f) {
            a /= length;
            b /= length;
            c /= length;
            d /= length;
        }
    }

    /**
     * @brief 计算点到平面的有向距离
     *
     * 距离 > 0: 点在平面前方（可见侧）
     * 距离 = 0: 点在平面上
     * 距离 < 0: 点在平面后方（被裁剪）
     *
     * @param point 待测试的点
     * @return 有向距离
     */
    float distanceToPoint(const Vector3& point) const {
        return a * point.x + b * point.y + c * point.z + d;
    }
};

/**
 * @brief 视锥体
 *
 * 由6个平面组成的视锥体，用于剔除不可见的物体
 *
 * 平面布局（法向量都指向视锥体内部）:
 *
 *         Top Plane
 *            ↑
 *            |    Near Plane
 *            |     ↓
 *   Left ---|-----|--- Right
 *            \    /
 *             \  /
 *              \/  ← Far Plane
 *
 */
struct Frustum {
    // ========================================================================
    // 6个裁剪平面
    // ========================================================================

    Plane left;    // 左平面
    Plane right;   // 右平面
    Plane top;     // 上平面
    Plane bottom;  // 下平面
    Plane near;    // 近平面
    Plane far;     // 远平面

    // ========================================================================
    // 视锥体角点（用于更精确的测试）
    // ========================================================================

    /** 近平面的4个角点 */
    Vector3 nearCorners[4];
    /** 远平面的4个角点 */
    Vector3 farCorners[4];

    enum CornerIndex {
        NearTopLeft = 0,
        NearTopRight,
        NearBottomLeft,
        NearBottomRight,
        FarTopLeft,
        FarTopRight,
        FarBottomLeft,
        FarBottomRight
    };

    // ========================================================================
    // 构造方法
    // ========================================================================

    Frustum() = default;

    /**
     * @brief 从视图-投影矩阵提取视锥体平面
     *
     * 原理:
     * VP矩阵的每一行对应一个平面的系数（在裁剪空间）
     * 通过矩阵变换可提取世界空间的平面方程
     *
     * 提取公式:
     * - Left Plane = Row3 + Row0
     * - Right Plane = Row3 - Row0
     * - Bottom Plane = Row3 + Row1
     * - Top Plane = Row3 - Row1
     * - Near Plane = Row3 + Row2
     * - Far Plane = Row3 - Row2
     *
     * @param viewProjectionMatrix 视图-投影矩阵
     * @return 构建的视锥体
     */
    static Frustum fromMatrix(const Matrix4& viewProjectionMatrix);

    /**
     * @brief 从相机参数构建视锥体
     *
     * @param position 相机位置
     * @param forward 相机前方向
     * @param up 相机上方向
     * @param right 相机右方向
     * @param nearDist 近平面距离
     * @param farDist 远平面距离
     * @param fov 垂直视场角（弧度）
     * @param aspect 宽高比
     * @return 构建的视锥体
     */
    static Frustum fromCamera(const Vector3& position,
                              const Vector3& forward,
                              const Vector3& up,
                              const Vector3& right,
                              float nearDist,
                              float farDist,
                              float fov,
                              float aspect);

    // ========================================================================
    // 包含测试
    // ========================================================================

    /**
     * @brief 测试点是否在视锥体内
     *
     * 点在视锥体内的条件:
     * 点在所有6个平面的前方（距离 >= 0）
     *
     * @param point 世界空间坐标
     * @return true 如果点在视锥体内或边界上
     */
    bool containsPoint(const Vector3& point) const;

    /**
     * @brief 测试球体是否与视锥体相交
     *
     * 优化: 使用球心到平面的距离与半径比较
     * - 如果距离 >= -radius，则球与平面相交或在平面内侧
     * - 如果所有平面都满足，则球在视锥体内
     *
     * @param center 球心（世界空间）
     * @param radius 球半径
     * @return true 如果球体与视锥体相交或完全包含在内
     */
    bool intersectsSphere(const Vector3& center, float radius) const;

    /**
     * @brief 测试轴对齐包围盒（AABB）是否与视锥体相交
     *
     * 方法: 对每个平面，找到AABB的"最负"顶点
     * 如果"最负"顶点在平面内侧，则AABB可能与视锥体相交
     *
     * @param min AABB最小点
     * @param max AABB最大点
     * @return true 如果AABB与视锥体相交或完全包含在内
     */
    bool intersectsAABB(const Vector3& min, const Vector3& max) const;

    /**
     * @brief 测试定向包围盒（OBB）是否与视锥体相交
     *
     * @param center OBB中心
     * @param halfExtents 半尺寸
     * @param rotation 旋转矩阵
     * @return true 如果OBB与视锥体相交或完全包含在内
     */
    bool intersectsOBB(const Vector3& center,
                       const Vector3& halfExtents,
                       const Matrix4& rotation) const;

    // ========================================================================
    // 分类测试
    // ========================================================================

    /**
     * @brief 物体与视锥体的关系
     */
    enum class IntersectionResult {
        Outside,   // 完全在外侧（不可见）
        Inside,    // 完全在内侧（可见）
        Intersect  // 相交（部分可见）
    };

    /**
     * @brief 详细测试球体与视锥体的关系
     * @param center 球心
     * @param radius 半径
     * @return 详细的相交结果
     */
    IntersectionResult classifySphere(const Vector3& center, float radius) const;

    /**
     * @brief 详细测试AABB与视锥体的关系
     * @param min AABB最小点
     * @param max AABB最大点
     * @return 详细的相交结果
     */
    IntersectionResult classifyAABB(const Vector3& min, const Vector3& max) const;

    // ========================================================================
    // 调试和可视化
    // ========================================================================

    /**
     * @brief 获取视锥体中心点
     */
    Vector3 getCenter() const;

    /**
     * @brief 获取视锥体的大致半径（用于粗略测试）
     */
    float getBoundingRadius() const;
};

/**
 * @brief 视锥剔除器
 *
 * 用于批量测试物体可见性的工具类
 */
class FrustumCuller {
public:
    /**
     * @brief 构建剔除器
     * @param frustum 视锥体
     */
    explicit FrustumCuller(const Frustum& frustum);

    /**
     * @brief 更新视锥体
     */
    void setFrustum(const Frustum& frustum);

    /**
     * @brief 测试物体的可见性（简化版）
     *
     * @param center 物体中心
     * @param radius 包围球半径
     * @return true 如果可见
     */
    bool isVisible(const Vector3& center, float radius) const;

    /**
     * @brief 批量测试物体的可见性
     *
     * @tparam T 物体类型，需要有 getCenter() 和 getRadius() 方法
     * @param objects 物体列表
     * @param outVisible 输出可见物体列表
     */
    template<typename T>
    void cull(const std::vector<T>& objects, std::vector<const T*>& outVisible) const {
        outVisible.clear();
        outVisible.reserve(objects.size());

        for (const auto& obj : objects) {
            if (isVisible(obj.getCenter(), obj.getRadius())) {
                outVisible.push_back(&obj);
            }
        }
    }

    /**
     * @brief 获取统计信息
     */
    struct Stats {
        uint32_t totalTested = 0;     // 总测试数
        uint32_t totalVisible = 0;    // 可见物体数
        uint32_t totalCulled = 0;     // 剔除物体数
    };
    const Stats& getStats() const { return stats_; }
    void resetStats() { stats_ = Stats(); }

private:
    Frustum frustum_;
    Stats stats_;
};
