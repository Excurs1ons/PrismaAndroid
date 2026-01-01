/**
 * @file Camera.h
 * @brief 相机组件 - 提供视图/投影矩阵计算和相机控制
 *
 * 参考概念: Unity Camera Component
 * 功能: 视图矩阵、投影矩阵、视锥体计算
 */

#pragma once

#include "../../Component.h"
#include "../../MathTypes.h"
#include <memory>

// 前向声明
struct Frustum;

/**
 * @brief 相机类型枚举
 */
enum class CameraType {
    /** 透视相机（默认，适用于3D场景） */
    Perspective,

    /** 正交相机（适用于2D场景、UI等） */
    Orthographic
};

/**
 * @brief 清除标志位
 */
enum class ClearFlag : uint32_t {
    /** 清除颜色缓冲区 */
    Color = 1 << 0,

    /** 清除深度缓冲区 */
    Depth = 1 << 1,

    /** 清除模板缓冲区 */
    Stencil = 1 << 2,

    /** 清除所有（Color | Depth | Stencil） */
    All = Color | Depth | Stencil
};

/** ClearFlag 的位运算操作符 */
inline ClearFlag operator|(ClearFlag a, ClearFlag b) {
    return static_cast<ClearFlag>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline ClearFlag operator&(ClearFlag a, ClearFlag b) {
    return static_cast<ClearFlag>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

/**
 * @brief 相机组件
 *
 * 继承自Component，可以添加到GameObject上
 *
 * 主要功能:
 * - 计算视图矩阵 (View Matrix)
 * - 计算投影矩阵 (Projection Matrix)
 * - 获取视锥体 (Frustum) 用于剔除
 * - 控制渲染顺序 (Depth)
 */
class Camera : public Component {
public:
    Camera();
    ~Camera() override = default;

    // ========================================================================
    // Component 接口实现
    // ========================================================================

    /** 每帧更新（用于平滑移动等） */
    void update(float deltaTime) override;

    // ========================================================================
    // 视图矩阵
    // ========================================================================

    /**
     * @brief 获取视图矩阵
     *
     * 视图矩阵将世界空间坐标转换到相机空间
     * View = Inverse(CameraTransform)
     *
     * @return 视图矩阵
     */
    Matrix4 getViewMatrix() const;

    /**
     * @brief 获取相机的前方向向量
     * @return 归一化的前方向向量
     */
    Vector3 getForward() const;

    /**
     * @brief 获取相机的上方向向量
     * @return 归一化的上方向向量
     */
    Vector3 getUp() const;

    /**
     * @brief 获取相机的右方向向量
     * @return 归一化的右方向向量
     */
    Vector3 getRight() const;

    // ========================================================================
    // 投影矩阵
    // ========================================================================

    /**
     * @brief 获取投影矩阵
     *
     * 透视投影矩阵构造方法:
     * f = 1.0 / tan(fov / 2)
     * [ f/aspect,  0,  0,                0              ]
     * [ 0,         f,  0,                0              ]
     * [ 0,         0,  (far+near)/(near-far), -1        ]
     * [ 0,         0,  (2*far*near)/(near-far), 0       ]
     *
     * @return 投影矩阵
     */
    Matrix4 getProjectionMatrix() const;

    /**
     * @brief 获取视图-投影矩阵
     *
     * VP = Projection * View
     *
     * @return 视图-投影矩阵
     */
    Matrix4 getViewProjectionMatrix() const;

    /**
     * @brief 获取逆视图-投影矩阵
     *
     * 用于将屏幕空间坐标转换到世界空间
     *
     * @return 逆视图-投影矩阵
     */
    Matrix4 getInvViewProjectionMatrix() const;

    // ========================================================================
    // 透视相机参数
    // ========================================================================

    /**
     * @brief 设置视场角（FOV）
     * @param fov 垂直视场角（度）
     */
    void setFieldOfView(float fov);

    /** @brief 获取视场角（度） */
    float getFieldOfView() const { return fieldOfView_; }

    /**
     * @brief 设置宽高比
     * @param aspect 宽高比 (width / height)
     */
    void setAspect(float aspect);

    /** @brief 获取宽高比 */
    float getAspect() const { return aspect_; }

    // ========================================================================
    // 正交相机参数
    // ========================================================================

    /**
     * @brief 设置正交相机的大小
     * @param size 正交大小（视口高度的一半）
     */
    void setOrthographicSize(float size);

    /** @brief 获取正交大小 */
    float getOrthographicSize() const { return orthographicSize_; }

    // ========================================================================
    // 裁剪平面（公共）
    // ========================================================================

    /**
     * @brief 设置近平面距离
     * @param nearPlane 近平面距离（必须 > 0）
     */
    void setNearPlane(float nearPlane);

    /** @brief 获取近平面距离 */
    float getNearPlane() const { return nearPlane_; }

    /**
     * @brief 设置远平面距离
     * @param farPlane 远平面距离（必须 > nearPlane）
     */
    void setFarPlane(float farPlane);

    /** @brief 获取远平面距离 */
    float getFarPlane() const { return farPlane_; }

    // ========================================================================
    // 视口
    // ========================================================================

    /**
     * @brief 设置视口矩形
     * @param x 视口左下角X坐标（归一化，0-1）
     * @param y 视口左下角Y坐标（归一化，0-1）
     * @param width 视口宽度（归一化，0-1）
     * @param height 视口高度（归一化，0-1）
     */
    void setViewport(float x, float y, float width, float height);

    /** @brief 获取视口X坐标 */
    float getViewportX() const { return viewportX_; }

    /** @brief 获取视口Y坐标 */
    float getViewportY() const { return viewportY_; }

    /** @brief 获取视口宽度 */
    float getViewportWidth() const { return viewportWidth_; }

    /** @brief 获取视口高度 */
    float getViewportHeight() const { return viewportHeight_; }

    // ========================================================================
    // 清除设置
    // ========================================================================

    /**
     * @brief 设置清除颜色
     * @param color 清除时的背景色
     */
    void setClearColor(const Vector3& color);

    /** @brief 获取清除颜色 */
    Vector3 getClearColor() const { return clearColor_; }

    /**
     * @brief 设置清除标志
     * @param flags 要清除的缓冲区
     */
    void setClearFlags(ClearFlag flags);

    /** @brief 获取清除标志 */
    ClearFlag getClearFlags() const { return clearFlags_; }

    // ========================================================================
    // 相机类型和优先级
    // ========================================================================

    /**
     * @brief 设置相机类型
     * @param type 透视或正交
     */
    void setCameraType(CameraType type);

    /** @brief 获取相机类型 */
    CameraType getCameraType() const { return cameraType_; }

    /**
     * @brief 设置渲染优先级
     *
     * 值越大越先渲染，多个相机时按此顺序渲染
     *
     * @param depth 渲染优先级
     */
    void setDepth(int depth);

    /** @brief 获取渲染优先级 */
    int getDepth() const { return depth_; }

    // ========================================================================
    // 视锥体
    // ========================================================================

    /**
     * @brief 获取视锥体
     *
     * 视锥体由6个平面组成:
     * - Near Plane (近平面)
     * - Far Plane (远平面)
     * - Left Plane (左平面)
     * - Right Plane (右平面)
     * - Top Plane (上平面)
     * - Bottom Plane (下平面)
     *
     * 用于视锥剔除（Frustum Culling）
     *
     * @return 视锥体结构
     */
    Frustum getFrustum() const;

    // ========================================================================
    // 屏幕空间转换
    // ========================================================================

    /**
     * @brief 将世界空间点转换到屏幕空间
     * @param worldPoint 世界空间坐标
     * @param screenWidth 屏幕宽度（像素）
     * @param screenHeight 屏幕高度（像素）
     * @return 屏幕空间坐标（像素，z为深度值0-1）
     */
    Vector3 worldToScreenPoint(const Vector3& worldPoint,
                                uint32_t screenWidth,
                                uint32_t screenHeight) const;

    /**
     * @brief 将屏幕空间点转换到世界空间
     * @param screenPoint 屏幕空间坐标（像素）
     * @param depth 深度值（0-1）
     * @param screenWidth 屏幕宽度（像素）
     * @param screenHeight 屏幕高度（像素）
     * @return 世界空间坐标
     */
    Vector3 screenToWorldPoint(const Vector2& screenPoint, float depth,
                                uint32_t screenWidth,
                                uint32_t screenHeight) const;

    /**
     * @brief 检查点是否在视锥体内
     * @param point 世界空间坐标
     * @return true 如果点可见
     */
    bool isPointVisible(const Vector3& point) const;

    /**
     * @brief 检查球体是否与视锥体相交
     * @param center 球心（世界空间）
     * @param radius 半径
     * @return true 如果球体可见
     */
    bool isSphereVisible(const Vector3& center, float radius) const;

    // ========================================================================
    // 射线检测
    // ========================================================================

    /**
     * @brief 从屏幕位置发射射线
     * @param screenPoint 屏幕空间坐标（像素）
     * @param screenWidth 屏幕宽度
     * @param screenHeight 屏幕高度
     * @return 射线（原点和方向）
     */
    struct Ray {
        Vector3 origin;
        Vector3 direction;
    };
    Ray screenPointToRay(const Vector2& screenPoint,
                          uint32_t screenWidth,
                          uint32_t screenHeight) const;

private:
    // ========================================================================
    // 成员变量
    // ========================================================================

    // 相机类型
    CameraType cameraType_ = CameraType::Perspective;

    // 透视相机参数
    float fieldOfView_ = 60.0f;     // 视场角（度）

    // 正交相机参数
    float orthographicSize_ = 10.0f; // 正交大小

    // 公共参数
    float aspect_ = 16.0f / 9.0f;    // 宽高比
    float nearPlane_ = 0.1f;         // 近平面距离
    float farPlane_ = 100.0f;        // 远平面距离

    // 视口（归一化坐标 0-1）
    float viewportX_ = 0.0f;
    float viewportY_ = 0.0f;
    float viewportWidth_ = 1.0f;
    float viewportHeight_ = 1.0f;

    // 清除设置
    Vector3 clearColor_ = Vector3(0.2f, 0.3f, 0.4f); // 默认天蓝色
    ClearFlag clearFlags_ = ClearFlag::All;

    // 渲染顺序
    int depth_ = 0;  // 渲染优先级

    // 缓存标记
    mutable bool viewMatrixDirty_ = true;
    mutable bool projectionMatrixDirty_ = true;

    // 缓存的矩阵
    mutable Matrix4 cachedViewMatrix_;
    mutable Matrix4 cachedProjectionMatrix_;
    mutable Matrix4 cachedViewProjectionMatrix_;
    mutable Matrix4 cachedInvViewProjectionMatrix_;
};
