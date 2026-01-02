/**
 * @file Architecture.md
 * @brief 基础渲染管线架构
 *
 * 参考 Unity URP 设计，采用 核心 Pass + 可扩展 Feature 的架构
 */

#pragma once

// ============================================================================
// 架构概览
// ============================================================================

/**
 * 设计原则:
 * - 核心 Pass 只负责基础渲染
 * - 所有扩展功能通过 IRenderFeature 实现
 * - Feature 可在任意渲染阶段插入
 */

// ============================================================================
// 渲染流程
// ============================================================================

/**
 * Frame Rendering:
 *
 * 1. BeforeRendering Features
 *
 * 2. ShadowPass (核心)
 *
 * 3. BeforeRenderingShadows / AfterRenderingShadows Features
 *
 * 4. BeforeRenderingOpaques Features
 *
 * 5. OpaquePass (核心) - PBR不透明物体
 *
 * 6. AfterRenderingOpaques Features (Bloom, SSAO, SSR...)
 *
 * 7. SkyboxPass (核心)
 *
 * 8. BeforeRenderingTransparents Features
 *
 * 9. TransparentPass (核心) - 透明物体
 *
 * 10. AfterRenderingTransparents Features
 *
 * 11. AfterRendering Features (PostProcess, AA...)
 *
 * 12. FinalBlit (核心)
 */

// ============================================================================
// 目录结构
// ============================================================================

/**
 * BasicPipeline/
 * ├── Architecture.md       # 本文件
 * ├── BasicRenderer.h       # 主渲染器
 * ├── IRenderFeature.h      # Feature接口
 * ├── Features.h            # Feature索引
 * ├── Features/             # Feature实现
 * │   ├── BloomFeature.h
 * │   ├── PostProcessFeature.h
 * │   ├── AntiAliasingFeature.h
 * │   ├── ScreenSpaceFeature.h
 * │   ├── DebugFeature.h
 * │   ├── DepthPrepassFeature.h
 * │   ├── UIFeature.h
 * │   ├── ReflectionFeature.h
 * │   └── VolumetricFeature.h
 * ├── RenderingData.h       # 渲染数据
 * ├── Camera.h              # 相机
 * ├── Frustum.h             # 视锥体
 * ├── RenderQueue.h         # 渲染队列
 * ├── DepthState.h          # 深度状态
 * ├── StencilState.h        # 模板状态
 * ├── ShadowSettings.h      # 阴影设置
 * ├── LightingData.h        # 光照数据
 * └── Passes/               # 核心Pass（只有5个）
 *     ├── OpaquePass.h
 *     ├── TransparentPass.h
 *     ├── SkyboxPass.h
 *     └── ShadowPass.h
 */

// ============================================================================
// 核心 Pass (只有5个)
// ============================================================================

/**
 * 核心Pass列表:
 *
 * - ShadowPass       渲染阴影贴图
 * - OpaquePass       渲染不透明物体 (PBR)
 * - SkyboxPass       渲染天空盒
 * - TransparentPass  渲染透明物体
 * - FinalBlitPass    输出到屏幕
 *
 * 所有其他效果都通过 Feature 实现
 */

// ============================================================================
// Feature 列表
// ============================================================================

/**
 * 后处理类:
 * - BloomFeature          泛光
 * - PostProcessFeature    色调映射、颜色分级
 * - AntiAliasingFeature   FXAA/TAA
 *
 * 屏幕空间类:
 * - SSAOFeature          环境光遮蔽
 * - SSRFeature           屏幕空间反射
 *
 * 调试类:
 * - DebugFeature         调试可视化
 *
 * 性能优化类:
 * - DepthPrepassFeature  深度预通过
 *
 * UI类:
 * - UIFeature            UI和文本
 *
 * 反射类:
 * - ReflectionProbeFeature    反射探针
 * - PlanarReflectionFeature   平面反射
 *
 * 体积效果类:
 * - VolumetricLightFeature    体积光
 * - VolumetricFogFeature      体积雾
 * - VolumetricCloudFeature    体积云
 */

// ============================================================================
// 使用示例
// ============================================================================

/**
 * @code
 *
 * // 创建渲染器
 * auto renderer = std::make_unique<BasicRenderer>();
 * renderer->Initialize(device, renderPass);
 *
 * // 添加Feature
 * renderer->AddFeature(std::make_unique<BloomFeature>());
 * renderer->AddFeature(std::make_unique<SSAOFeature>());
 * renderer->AddFeature(std::make_unique<PostProcessFeature>());
 *
 * // 渲染
 * renderer->Render(scene, camera, cmdBuffer);
 *
 * @endcode
 */
