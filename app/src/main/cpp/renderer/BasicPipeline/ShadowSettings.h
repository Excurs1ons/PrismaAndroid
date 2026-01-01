/**
 * @file ShadowSettings.h
 * @brief 阴影渲染设置
 *
 * 支持多种阴影技术:
 * - Shadow Mapping (阴影贴图)
 * - PCF (Percentage Closer Filtering) 软阴影
 * - Cascaded Shadow Maps (CSM) 级联阴影
 * - Point Light Shadow (立方体阴影贴图)
 */

#pragma once

#include <vector>
#include <cstdint>

/**
 * @brief 阴影类型
 */
enum class ShadowType {
    /** 无阴影 */
    None = 0,

    /** 硬阴影（Shadow Mapping，无过滤） */
    HardShadows = 1,

    /** 软阴影（PCF 2x2） */
    SoftShadows = 2,

    /** 高质量软阴影（PCF 4x4 或 POISSON采样） */
    HighQualitySoftShadows = 3
};

/**
 * @brief 阴影分辨率
 */
enum class ShadowResolution {
    /** 低分辨率 (512x512) - 适用于移动设备 */
    Low = 512,

    /** 中分辨率 (1024x1024) - 默认 */
    Medium = 1024,

    /** 高分辨率 (2048x2048) */
    High = 2048,

    /** 超高分辨率 (4096x4096) */
    Ultra = 4096
};

/**
 * @brief 阴影贴图类型
 */
enum class ShadowMapType {
    /** 单张阴影贴图（定向光） */
    Single = 0,

    /** 立方体阴影贴图（点光源） */
    Cubemap = 1,

    /** 级联阴影贴图（Cascaded Shadow Maps） */
    Cascaded = 2
};

/**
 * @brief 单个光源的阴影设置
 */
struct PerLightShadowSettings {
    /** 是否启用此光源的阴影 */
    bool enabled = true;

    /** 阴影贴图分辨率 */
    ShadowResolution resolution = ShadowResolution::Medium;

    /** 阴影类型 */
    ShadowType type = ShadowType::SoftShadows;

    /** 阴影偏移（避免Shadow Acne） */
    float bias = 0.005f;

    /** 法线偏移强度（基于表面法线） */
    float normalBias = 0.1f;

    /** 近平面距离 */
    float nearPlane = 0.1f;

    /** 远平面距离 */
    float farPlane = 100.0f;

    /** 阴影强度 (0.0 - 1.0) */
    float strength = 0.8f;

    /**
     * @brief 获取默认设置
     */
    static PerLightShadowSettings defaultSettings() {
        return PerLightShadowSettings{};
    }
};

/**
 * @brief 级联阴影贴图 (CSM) 设置
 *
 * 用于大场景的高质量阴影
 * 将视锥体分割为多个级联，每个级联使用独立的阴影贴图
 */
struct CascadedShadowSettings {
    /** 级联数量 (1-4) */
    uint32_t cascadeCount = 4;

    /** 级联分割方案 */
    enum class SplitScheme {
        /** 均匀分割 */
        Uniform,

        /** 对数分割（更适合透视） */
        Logarithmic,

        /** 手动分割比例 */
        Manual,

        /** 混合方案（常用） */
        PseudoLogarithmic
    };
    SplitScheme splitScheme = SplitScheme::PseudoLogarithmic;

    /**
     * 手动分割比例（当 splitScheme = Manual 时使用）
     * 数组长度应为 cascadeCount - 1
     * 例如: {0.05f, 0.15f, 0.40f} 表示 4个级联的分割点
     */
    std::vector<float> manualSplits;

    /** 每个级联的阴影分辨率 */
    ShadowResolution resolution = ShadowResolution::Medium;

    /** 级联之间的过渡区域大小 (0.0 - 1.0) */
    float transitionSize = 0.1f;

    /** 是否启用级联混合（消除级联边界） */
    bool enableCascadeBlending = true;

    /**
     * @brief 获取默认的4级联设置
     */
    static CascadedShadowSettings default4Cascades() {
        CascadedShadowSettings settings;
        settings.cascadeCount = 4;
        settings.splitScheme = SplitScheme::PseudoLogarithmic;
        settings.resolution = ShadowResolution::Medium;
        settings.transitionSize = 0.1f;
        settings.enableCascadeBlending = true;
        return settings;
    }

    /**
     * @brief 计算级联分割距离
     *
     * @param nearPlane 近平面距离
     * @param farPlane 远平面距离
     * @return 级联分割距离数组（长度 = cascadeCount + 1）
     */
    std::vector<float> calculateSplitDistances(float nearPlane, float farPlane) const;
};

/**
 * @brief 阴影过滤设置
 */
struct ShadowFilterSettings {
    /** 过滤类型 */
    enum class FilterType {
        /** 无过滤（硬阴影） */
        None = 0,

        /** PCF 2x2 */
        PCF2x2 = 1,

        /** PCF 3x3 */
        PCF3x3 = 2,

        /** PCF 4x4 */
        PCF4x4 = 3,

        /** PCF 5x5 */
        PCF5x5 = 4,

        /** POISSON采样（高质量软阴影） */
        Poisson = 5,

        /** PCSS (Percentage Closer Soft Shadows) */
        PCSS = 6
    };

    FilterType filterType = FilterType::PCF2x2;

    /** 采样半径（仅用于 Poisson/PCSS） */
    float sampleRadius = 1.5f;

    /** 采样数量（仅用于 Poisson） */
    uint32_t sampleCount = 16;

    /**
     * @brief 获取默认过滤设置
     */
    static ShadowFilterSettings defaultPCF() {
        ShadowFilterSettings settings;
        settings.filterType = FilterType::PCF2x2;
        settings.sampleRadius = 1.0f;
        settings.sampleCount = 4;
        return settings;
    }
};

/**
 * @brief 全局阴影设置
 *
 * 控制整个渲染管线的阴影行为
 */
struct ShadowSettings {
    // ========================================================================
    // 全局开关
    // ========================================================================

    /** 是否启用阴影系统 */
    bool enableShadows = true;

    /** 默认阴影类型 */
    ShadowType defaultShadowType = ShadowType::SoftShadows;

    // ========================================================================
    // 阴影贴图资源
    // ========================================================================

    /** 最大阴影贴图数量（所有光源共享） */
    uint32_t maxShadowMaps = 16;

    /** 阴影贴图数组的大小 */
    uint32_t shadowMapArraySize = 8;

    // ========================================================================
    // 距离设置
    // ========================================================================

    /** 阴影渲染距离（世界单位） */
    float shadowDistance = 50.0f;

    /** 阴影淡出距离（在shadowDistance附近淡出） */
    float shadowFadeDistance = 10.0f;

    // ========================================================================
    // 级联阴影（定向光）
    // ========================================================================

    /** 是否启用级联阴影 */
    bool enableCascadedShadows = true;

    /** 级联阴影设置 */
    CascadedShadowSettings cascadedSettings;

    // ========================================================================
    // 过滤设置
    // ========================================================================

    /** 阴影过滤设置 */
    ShadowFilterSettings filterSettings;

    // ========================================================================
    // 性能设置
    // ========================================================================

    /** 每帧最多渲染的阴影光源数量 */
    uint32_t maxShadowCastingLightsPerFrame = 4;

    /** 是否启用阴影剔除（剔除阴影渲染视锥外的物体） */
    bool enableShadowCulling = true;

    /** 是否使用深度预通过优化阴影渲染 */
    bool enableDepthPrepassForShadows = false;

    // ========================================================================
    // 质量设置
    // ========================================================================

    /** 是否使用双向深度偏移（防止Shadow Acne和Peter Panning） */
    bool useBidirectionalDepthBias = true;

    /** 深度偏移缩放 */
    float depthBiasScale = 1.0f;

    /** 法线偏移缩放 */
    float normalBiasScale = 1.0f;

    // ========================================================================
    // 预设配置
    // ========================================================================

    /**
     * @brief 获取默认设置（桌面平台）
     */
    static ShadowSettings defaultSettings() {
        ShadowSettings settings;
        settings.enableShadows = true;
        settings.defaultShadowType = ShadowType::SoftShadows;
        settings.maxShadowMaps = 16;
        settings.shadowMapArraySize = 8;
        settings.shadowDistance = 50.0f;
        settings.shadowFadeDistance = 10.0f;
        settings.enableCascadedShadows = true;
        settings.cascadedSettings = CascadedShadowSettings::default4Cascades();
        settings.filterSettings = ShadowFilterSettings::defaultPCF();
        settings.maxShadowCastingLightsPerFrame = 4;
        settings.enableShadowCulling = true;
        settings.useBidirectionalDepthBias = true;
        return settings;
    }

    /**
     * @brief 获取移动端设置（性能优先）
     */
    static ShadowSettings mobileSettings() {
        ShadowSettings settings;
        settings.enableShadows = true;
        settings.defaultShadowType = ShadowType::HardShadows;
        settings.maxShadowMaps = 4;
        settings.shadowMapArraySize = 4;
        settings.shadowDistance = 30.0f;
        settings.shadowFadeDistance = 5.0f;
        settings.enableCascadedShadows = false;  // 禁用CSM
        settings.cascadedSettings.cascadeCount = 1;
        settings.filterSettings.filterType = ShadowFilterSettings::FilterType::None;
        settings.maxShadowCastingLightsPerFrame = 1;  // 单光源阴影
        settings.enableShadowCulling = true;
        return settings;
    }

    /**
     * @brief 获取高质量设置（性能不敏感）
     */
    static ShadowSettings highQualitySettings() {
        ShadowSettings settings = defaultSettings();
        settings.defaultShadowType = ShadowType::HighQualitySoftShadows;
        settings.maxShadowMaps = 32;
        settings.shadowMapArraySize = 16;
        settings.shadowDistance = 100.0f;
        settings.cascadedSettings.resolution = ShadowResolution::High;
        settings.filterSettings.filterType = ShadowFilterSettings::FilterType::Poisson;
        settings.filterSettings.sampleCount = 32;
        settings.maxShadowCastingLightsPerFrame = 8;
        return settings;
    }

    // ========================================================================
    // 工具方法
    // ========================================================================

    /**
     * @brief 检查是否应该渲染此光源的阴影
     * @param lightIndex 光源索引
     * @return true 如果应该渲染
     */
    bool shouldRenderShadow(int lightIndex) const {
        return enableShadows && lightIndex < static_cast<int>(maxShadowCastingLightsPerFrame);
    }

    /**
     * @brief 计算阴影淡出因子
     * @param distance 到相机的距离
     * @return 淡出因子 (1.0 = 完全可见, 0.0 = 完全透明)
     */
    float calculateShadowFade(float distance) const {
        if (distance >= shadowDistance) {
            return 0.0f;
        }
        if (distance <= shadowDistance - shadowFadeDistance) {
            return 1.0f;
        }
        float fadeRange = shadowFadeDistance;
        float fadeStart = shadowDistance - fadeRange;
        return 1.0f - (distance - fadeStart) / fadeRange;
    }
};

/**
 * @brief 阴影贴图 atlas 信息
 *
 * 用于管理多个光源的阴影贴图打包
 */
struct ShadowAtlas {
    /** Atlas 宽度 */
    uint32_t width = 2048;

    /** Atlas 高度 */
    uint32_t height = 2048;

    /** 当前使用的区域 */
    struct Rect {
        uint32_t x, y;
        uint32_t width, height;
    };
    std::vector<Rect> allocatedRects;

    /**
     * @brief 分配一个新的阴影贴图区域
     * @param shadowWidth 阴影贴图宽度
     * @param shadowHeight 阴影贴图高度
     * @return 分配的矩形，如果失败则返回无效矩形
     */
    Rect allocate(uint32_t shadowWidth, uint32_t shadowHeight);

    /**
     * @brief 重置 Atlas（清空所有分配）
     */
    void reset();
};
