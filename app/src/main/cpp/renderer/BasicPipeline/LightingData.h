/**
 * @file LightingData.h
 * @brief 光照数据定义
 *
 * 支持多种光源类型:
 * - Directional Light (定向光/平行光)
 * - Point Light (点光源)
 * - Spot Light (聚光灯)
 * - Area Light (区域光)
 */

#pragma once

#include "../MathTypes.h"
#include <vector>
#include <memory>

/**
 * @brief 光源类型
 */
enum class LightType {
    /** 定向光（太阳光，无位置，只有方向） */
    Directional = 0,

    /** 点光源（灯泡，从一点向所有方向发光） */
    Point = 1,

    /** 聚光灯（手电筒，锥形光照） */
    Spot = 2,

    /** 区域光（面光源，高级） */
    Area = 3
};

/**
 * @brief 光照模式
 */
enum class LightMode {
    /** 实时光照 */
    Realtime = 0,

    /** 烘焙光照（Lightmap） */
    Baked = 1,

    /** 混合模式
    */
    Mixed = 2
};

/**
 * @brief 光影投射类型
 */
enum class ShadowCastingMode {
    /** 不投射阴影 */
    Off = 0,

    /** 只投射阴影（自身不受光照影响） */
    ShadowsOnly = 1,

    /** 投射阴影并受光照影响（标准） */
    On = 2
};

/**
 * @brief 基础光源数据
 */
struct LightData {
    // ========================================================================
    // 通用属性
    // ========================================================================

    /** 光源类型 */
    LightType type = LightType::Point;

    /** 光源颜色 (RGB, 0-1) */
    Vector3 color = Vector3(1.0f, 1.0f, 1.0f);

    /** 光照强度 */
    float intensity = 1.0f;

    /** 光照范围（仅用于点光源和聚光灯） */
    float range = 10.0f;

    // ========================================================================
    // 定向光属性
    // ========================================================================

    /** 光照方向（仅用于定向光，归一化） */
    Vector3 direction = Vector3(0.0f, -1.0f, 0.0f);

    // ========================================================================
    // 点光源属性
    // ========================================================================

    /** 光源位置（世界空间） */
    Vector3 position = Vector3(0.0f, 0.0f, 0.0f);

    /** 衰减类型 */
    enum class Attenuation {
        /** 线性衰减 */
        Linear,

        /** 平方反比衰减（物理准确） */
        InverseSquare,

        /** 自定义衰减曲线 */
        Custom
    };
    Attenuation attenuation = Attenuation::InverseSquare;

    // ========================================================================
    // 聚光灯属性
    // ========================================================================

    /** 聚光灯内角（度）*/
    float innerAngle = 15.0f;

    /** 聚光灯外角（度，边缘柔化） */
    float outerAngle = 30.0f;

    // ========================================================================
    // 阴影
    // ========================================================================

    /** 是否投射阴影 */
    bool castShadows = false;

    /** 阴影强度 (0.0 - 1.0) */
    float shadowStrength = 1.0f;

    /** 阴影偏移 */
    float shadowBias = 0.005f;

    /** 阴影近平面 */
    float shadowNearPlane = 0.1f;

    // ========================================================================
    // 高级
    // ========================================================================

    /** 光照模式 */
    LightMode lightMode = LightMode::Realtime;

    /** 是否影响光照贴图物体 */
    bool affectLightmappedSurfaces = true;

    // ========================================================================
    // 工具方法
    // ========================================================================

    /**
     * @brief 创建定向光
     */
    static LightData createDirectional(const Vector3& direction,
                                       const Vector3& color = Vector3(1.0f),
                                       float intensity = 1.0f) {
        LightData light;
        light.type = LightType::Directional;
        light.direction = glm::normalize(direction);
        light.color = color;
        light.intensity = intensity;
        return light;
    }

    /**
     * @brief 创建点光源
     */
    static LightData createPoint(const Vector3& position,
                                 float range,
                                 const Vector3& color = Vector3(1.0f),
                                 float intensity = 1.0f) {
        LightData light;
        light.type = LightType::Point;
        light.position = position;
        light.range = range;
        light.color = color;
        light.intensity = intensity;
        light.attenuation = Attenuation::InverseSquare;
        return light;
    }

    /**
     * @brief 创建聚光灯
     */
    static LightData createSpot(const Vector3& position,
                               const Vector3& direction,
                               float innerAngle,
                               float outerAngle,
                               float range,
                               const Vector3& color = Vector3(1.0f),
                               float intensity = 1.0f) {
        LightData light;
        light.type = LightType::Spot;
        light.position = position;
        light.direction = glm::normalize(direction);
        light.innerAngle = innerAngle;
        light.outerAngle = outerAngle;
        light.range = range;
        light.color = color;
        light.intensity = intensity;
        return light;
    }

    /**
     * @brief 计算衰减因子
     * @param distance 到光源的距离
     * @return 衰减因子 (0.0 - 1.0)
     */
    float calculateAttenuation(float distance) const {
        if (type == LightType::Directional) {
            return 1.0f;  // 定向光无衰减
        }

        if (distance >= range) {
            return 0.0f;
        }

        switch (attenuation) {
            case Attenuation::Linear:
                return 1.0f - (distance / range);

            case Attenuation::InverseSquare:
                // 使用改进的平方反比公式，避免无限远处的问题
                float d = distance / range;
                return 1.0f / (1.0f + d * d);

            case Attenuation::Custom:
                // TODO: 实现自定义衰减曲线
                return 1.0f - (distance / range);
        }
        return 0.0f;
    }
};

/**
 * @brief 场景光照数据容器
 *
 * 收集场景中所有的光源，供渲染管线使用
 */
struct LightingData {
    // ========================================================================
    // 光源列表
    // ========================================================================

    /** 所有定向光 */
    std::vector<LightData> directionalLights;

    /** 所有点光源 */
    std::vector<LightData> pointLights;

    /** 所有聚光灯 */
    std::vector<LightData> spotLights;

    // ========================================================================
    // 环境光
    // ========================================================================

    /** 环境光颜色 */
    Vector3 ambientColor = Vector3(0.1f, 0.1f, 0.15f);

    /** 环境光强度 */
    float ambientIntensity = 0.3f;

    /** 环境光光照贴图（如果有） */
    void* ambientLightmap = nullptr;  // API相关的纹理句柄

    // ========================================================================
    // 全局光照
    // ========================================================================

    /** 是否启用全局光照 */
    bool enableGI = false;

    /** 光照探针数据 */
    struct LightProbe {
        Vector3 position;
        Vector3 sphericalHarmonics[9];  // 三阶球谐函数
    };
    std::vector<LightProbe> lightProbes;

    // ========================================================================
    // 配置
    // ========================================================================

    /** 每帧最多渲染的光源数量（性能考虑） */
    uint32_t maxLightsPerFrame = 16;

    /** 点光源的最大影响范围 */
    float maxLightRange = 50.0f;

    // ========================================================================
    // 工具方法
    // ========================================================================

    /**
     * @brief 添加光源
     */
    void addLight(const LightData& light) {
        switch (light.type) {
            case LightType::Directional:
                directionalLights.push_back(light);
                break;
            case LightType::Point:
                pointLights.push_back(light);
                break;
            case LightType::Spot:
                spotLights.push_back(light);
                break;
            case LightType::Area:
                // 暂不支持
                break;
        }
    }

    /**
     * @brief 清空所有光源
     */
    void clear() {
        directionalLights.clear();
        pointLights.clear();
        spotLights.clear();
        lightProbes.clear();
    }

    /**
     * @brief 获取最重要的光源（用于光照计算）
     *
     * 返回最亮的和最近的光源
     *
     * @param position 世界空间位置
     * @param maxCount 最大返回数量
     * @return 重要光源列表
     */
    std::vector<const LightData*> getImportantLights(const Vector3& position,
                                                      uint32_t maxCount = 4) const;

    /**
     * @brief 计算某一点的总光照（用于光照探针更新等）
     * @param position 世界空间位置
     * @param normal 表面法线
     * @return 总光照颜色
     */
    Vector3 calculateLightingAtPoint(const Vector3& position,
                                     const Vector3& normal) const;
};

/**
 * @brief 光照组件（可添加到GameObject）
 */
class LightComponent {
public:
    LightComponent() = default;
    ~LightComponent() = default;

    /** 光源数据 */
    LightData lightData;

    /** 光源是否激活 */
    bool isActive = true;

    /**
     * @brief 更新光源位置（从GameObject获取）
     */
    void updateFromTransform(const Vector3& position, const Vector3& rotation);

    /**
     * @brief 获取用于渲染的光源数据
     */
    const LightData& getData() const { return lightData; }
};
