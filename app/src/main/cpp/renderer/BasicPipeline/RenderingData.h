/**
 * @file RenderingData.h
 * @brief 渲染数据配置 - 包含单次渲染所需的所有配置参数
 *
 * 参考概念: Unity URP RenderingData
 * 用途: 作为渲染管线的主数据容器，在各个Pass之间传递渲染配置
 */

#pragma once

#include "../../MathTypes.h"
#include <memory>
#include <vector>

// 前向声明
class Camera;
struct LightingData;
struct ShadowSettings;
struct DrawSettings;

/**
 * @brief 渲染数据容器
 *
 * 包含单次渲染（通常是一帧）所需的所有配置数据。
 * 在渲染管线初始化时构建，传递给各个RenderPass使用。
 *
 * 设计理念:
 * - 数据驱动: 所有的渲染配置都通过这个结构体传递
 * - 不可变性: 构建后不应修改（线程安全）
 * - 传递高效: 使用指针/引用避免数据拷贝
 */
struct RenderingData {
    // ========================================================================
    // 相机数据
    // ========================================================================

    /** 当前渲染的相机（可能有多个相机进行渲染） */
    Camera* camera = nullptr;

    /** 相机的视图矩阵 */
    Matrix4 viewMatrix;

    /** 相机的投影矩阵 */
    Matrix4 projectionMatrix;

    /** 视图-投影矩阵（预先计算，避免重复计算） */
    Matrix4 viewProjectionMatrix;

    /** 相机的世界空间位置（用于着色器计算） */
    Vector3 cameraPosition;

    // ========================================================================
    // 时间数据
    // ========================================================================

    /** 当前帧的时间（秒） */
    float time = 0.0f;

    /** 上一帧到当前帧的增量时间（秒） */
    float deltaTime = 0.0f;

    // ========================================================================
    // 光照和阴影
    // ========================================================================

    /** 场景光照数据 */
    LightingData* lightingData = nullptr;

    /** 阴影设置 */
    ShadowSettings* shadowSettings = nullptr;

    // ========================================================================
    // 渲染目标
    // ========================================================================

    /** 渲染目标宽度 */
    uint32_t screenWidth = 0;

    /** 渲染目标高度 */
    uint32_t screenHeight = 0;

    // ========================================================================
    // 调试和开关
    // ========================================================================

    /** 是否启用阴影 */
    bool enableShadows = true;

    /** 是否启用后处理 */
    bool enablePostProcessing = true;

    /** 是否启用调试视图 */
    bool debugView = false;

    // ========================================================================
    // 工厂方法
    // ========================================================================

    /**
     * @brief 创建默认的RenderingData
     */
    static RenderingData create() {
        RenderingData data;
        data.viewMatrix = Matrix4(1.0f);
        data.projectionMatrix = Matrix4(1.0f);
        data.viewProjectionMatrix = Matrix4(1.0f);
        data.cameraPosition = Vector3(0.0f);
        return data;
    }
};

/**
 * @brief 单个Pass的渲染数据
 *
 * 某个特定RenderPass所需的渲染数据，是RenderingData的子集
 */
struct PassRenderData {
    /** Pass名称（用于调试和日志） */
    const char* passName = "Unnamed Pass";

    /** 引用完整的渲染数据 */
    const RenderingData* renderingData = nullptr;

    /** 当前Pass的命令缓冲区（API相关） */
    void* commandBuffer = nullptr;

    /** 当前帧索引（用于双缓冲/三缓冲） */
    uint32_t currentFrameIndex = 0;
};
