/**
 * @file Architecture.md
 * @brief 基础渲染管线架构规划
 *
 * 参考现代渲染管线设计理念（如Unity URP、Unreal渲染架构）
 *
 * ============================================================================
 * 架构概览
 * ============================================================================
 *
 * 本实现提供可扩展的渲染管线架构：
 *
 * 1. RenderingData: 单次渲染的所有配置数据（相机、光照、阴影等）
 * 2. RenderData: 某个 Pass 所需的渲染数据
 * 3. Camera: 相机组件（视图/投影矩阵、视锥剔除）
 * 4. RenderQueue: 渲染队列（排序、剔除、批量）
 * 5. DepthState: 深度测试状态配置
 * 6. StencilState: 模板测试状态配置
 * 7. ShadowSettings: 阴影设置
 * 8. LightingData: 光照数据
 *
 * ============================================================================
 * 渲染流程
 * ============================================================================
 *
 * Frame Rendering:
 *   1. Setup
 *      - 收集场景中的所有相机、光源、渲染器
 *      - 构建 RenderingData
 *
 *   2. Per-Camera Rendering
 *      a. Camera Setup
 *         - 计算视图/投影矩阵
 *         - 视锥剔除 (Frustum Culling)
 *         - 构建渲染队列 (RenderQueue)
 *
 *      b. Shadow Pass
 *         - 渲染阴影贴图
 *
 *      c. Opaque Pass
 *         - 深度测试开启
 *         - 从前到后排序（Early-Z优化）
 *         - 渲染不透明物体
 *
 *      d. Skybox Pass
 *         - 渲染天空盒
 *
 *      e. Transparent Pass
 *         - 深度测试开启，深度写入关闭
 *         - 从后到前排序
 *         - 渲染透明物体
 *
 *      f. Post-Processing Pass
 *         - 应用后处理效果
 *
 * ============================================================================
 * 目录结构
 * ============================================================================
 *
 * renderer/BasicPipeline/
 * ├── Architecture.md           (本文件 - 架构文档)
 * ├── RenderingData.h          (渲染数据配置)
 * ├── Camera.h                 (相机组件)
 * ├── Frustum.h                (视锥体)
 * ├── RenderQueue.h            (渲染队列)
 * ├── DepthState.h             (深度状态)
 * ├── StencilState.h           (模板状态)
 * ├── ShadowSettings.h         (阴影设置)
 * ├── LightingData.h           (光照数据)
 * └── Passes/
 *     ├── ShadowPass.h         (阴影渲染Pass)
 *     ├── OpaquePass.h         (不透明Pass)
 *     ├── TransparentPass.h    (透明Pass)
 *     └── PostProcessPass.h    (后处理Pass)
 */
