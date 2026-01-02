/**
 * @file Features.h
 * @brief 渲染特性总索引
 *
 * 本文件包含所有可用的渲染Feature
 * 按功能分类
 */

#pragma once

#include "IRenderFeature.h"

// ============================================================================
// 后处理类
// ============================================================================
#include "Features/BloomFeature.h"
#include "Features/PostProcessFeature.h"
#include "Features/AntiAliasingFeature.h"

// ============================================================================
// 屏幕空间类
// ============================================================================
#include "Features/ScreenSpaceFeature.h"

// ============================================================================
// 调试类
// ============================================================================
#include "Features/DebugFeature.h"

// ============================================================================
// 性能优化类
// ============================================================================
#include "Features/DepthPrepassFeature.h"

// ============================================================================
// UI类
// ============================================================================
#include "Features/UIFeature.h"

// ============================================================================
// 反射类
// ============================================================================
#include "Features/ReflectionFeature.h"

// ============================================================================
// 体积效果类
// ============================================================================
#include "Features/VolumetricFeature.h"

/**
 * @brief 创建默认的Feature集合
 *
 * 返回一个包含常用Feature的渲染器
 */
inline std::vector<std::unique_ptr<IRenderFeature>> CreateDefaultFeatures() {
    std::vector<std::unique_ptr<IRenderFeature>> features;

    // 深度预通过
    features.push_back(std::make_unique<DepthPrepassFeature>());

    // SSAO
    features.push_back(std::make_unique<SSAOFeature>());

    // 后处理
    features.push_back(std::make_unique<PostProcessFeature>());

    // 抗锯齿
    features.push_back(std::make_unique<AntiAliasingFeature>());

    // UI
    features.push_back(std::make_unique<UIFeature>());

    return features;
}

/**
 * @brief 创建高质量Feature集合
 */
inline std::vector<std::unique_ptr<IRenderFeature>> CreateHighQualityFeatures() {
    std::vector<std::unique_ptr<IRenderFeature>> features;

    features.push_back(std::make_unique<DepthPrepassFeature>());
    features.push_back(std::make_unique<SSAOFeature>());
    features.push_back(std::make_unique<SSRFeature>());
    features.push_back(std::make_unique<BloomFeature>());
    features.push_back(std::make_unique<VolumetricLightFeature>());
    features.push_back(std::make_unique<PostProcessFeature>());
    features.push_back(std::make_unique<AntiAliasingFeature>());
    features.push_back(std::make_unique<UIFeature>());
    features.push_back(std::make_unique<ReflectionProbeFeature>());

    return features;
}

/**
 * @brief 创建移动端Feature集合（精简）
 */
inline std::vector<std::unique_ptr<IRenderFeature>> CreateMobileFeatures() {
    std::vector<std::unique_ptr<IRenderFeature>> features;

    // 移动端只保留必要的效果
    features.push_back(std::make_unique<PostProcessFeature>());
    features.push_back(std::make_unique<UIFeature>());

    return features;
}
