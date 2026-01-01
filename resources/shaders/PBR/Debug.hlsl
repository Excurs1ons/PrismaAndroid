/**
 * @file Debug.hlsl
 * @brief 调试可视化Shader
 *
 * 用于渲染管线调试和可视化
 *
 * 支持的调试模式:
 * - None: 正常渲染
 * - Wireframe: 线框模式
 * - Normals: 法线可视化
 * - Tangents: 切线可视化
 * - Bitangents: 副切线可视化
 * - UV: UV坐标可视化
 * - UV2: 第二套UV坐标
 * - VertexColors: 顶点颜色可视化
 * - Depth: 深度可视化
 * - LinearDepth: 线性深度可视化
 * - Albedo: 反照率可视化
 * - Metallic: 金属度可视化
 * - Roughness: 粗糙度可视化
 * - AO: 环境光遮蔽可视化
 * - Emission: 发光可视化
 * - Specular: 镜面反射可视化
 * - FaceNormals: 面法线可视化（三角形着色）
 * - MeshBounds: 网格包围盒可视化
 * - Bounds: 物体包围盒可视化
 */

#ifndef DEBUG_HLSL
#define DEBUG_HLSL

// ============================================================================
// 调试模式枚举
// ============================================================================

#define DEBUG_MODE_NONE 0
#define DEBUG_MODE_WIREFRAME 1
#define DEBUG_MODE_NORMALS 2
#define DEBUG_MODE_TANGENTS 3
#define DEBUG_MODE_BITANGENTS 4
#define DEBUG_MODE_UV 5
#define DEBUG_MODE_UV2 6
#define DEBUG_MODE_VERTEX_COLORS 7
#define DEBUG_MODE_DEPTH 8
#define DEBUG_MODE_LINEAR_DEPTH 9
#define DEBUG_MODE_ALBEDO 10
#define DEBUG_MODE_METALLIC 11
#define DEBUG_MODE_ROUGHNESS 12
#define DEBUG_MODE_AO 13
#define DEBUG_MODE_EMISSION 14
#define DEBUG_MODE_SPECULAR 15
#define DEBUG_MODE_FACE_NORMALS 16
#define DEBUG_MODE_MESH_BOUNDS 17
#define DEBUG_MODE_BOUNDS 18

// 运行时选择的调试模式（通过uniform设置）
uniform int g_DebugMode = DEBUG_MODE_NONE;

// 调试显示强度
uniform float g_DebugIntensity = 1.0;

// 是否显示热力图颜色
uniform bool g_DebugHeatmap = false;

// ============================================================================
// 输入结构
// ============================================================================

struct DebugInput {
    // 顶点数据
    float3 positionWS;      // 世界空间位置
    float3 positionVS;      // 视图空间位置
    float3 normalWS;        // 世界空间法线
    float3 normalVS;        // 视图空间法线
    float3 tangentWS;       // 世界空间切线
    float3 bitangentWS;     // 世界空间副切线
    float2 uv;              // 纹理坐标
    float2 uv2;             // 第二套纹理坐标
    float4 color;           // 顶点颜色

    // 材质数据
    float3 albedo;          // 反照率
    float metallic;         // 金属度
    float roughness;        // 粗糙度
    float ao;               // 环境光遮蔽
    float3 emission;        // 发光
    float3 specular;        // 镜面反射

    // 深度数据
    float depth;            // 非线性深度
    float linearDepth;      // 线性深度

    // 面数据（用于面法线）
    float3 faceNormalWS;    // 面法线（世界空间）
    float3 faceNormalVS;    // 面法线（视图空间）
};

// ============================================================================
// 辅助函数
// ============================================================================

/**
 * @brief 将值映射到热力图颜色
 *
 * 蓝色(冷) -> 绿色 -> 黄色 -> 红色(热)
 *
 * @param t 输入值 (0-1)
 * @return 热力图颜色
 */
float3 HeatmapColor(float t) {
    const float3 kBlue = float3(0.0, 0.0, 1.0);
    const float3 kCyan = float3(0.0, 1.0, 1.0);
    const float3 kGreen = float3(0.0, 1.0, 0.0);
    const float3 kYellow = float3(1.0, 1.0, 0.0);
    const float3 kRed = float3(1.0, 0.0, 0.0);

    t = saturate(t);

    if (t < 0.25) {
        return lerp(kBlue, kCyan, t * 4.0);
    } else if (t < 0.5) {
        return lerp(kCyan, kGreen, (t - 0.25) * 4.0);
    } else if (t < 0.75) {
        return lerp(kGreen, kYellow, (t - 0.5) * 4.0);
    } else {
        return lerp(kYellow, kRed, (t - 0.75) * 4.0);
    }
}

/**
 * @brief 将法线转换为可视化颜色
 *
 * @param normal 归一化法线
 * @return RGB颜色 (映射到0-1范围)
 */
float3 NormalToColor(float3 normal) {
    return normal * 0.5 + 0.5;
}

/**
 * @brief 将UV坐标转换为可视化颜色
 *
 * @param uv UV坐标
 * @return 可视化颜色 (RG=UV, B=棋盘格)
 */
float3 UVToColor(float2 uv) {
    float3 color;
    color.r = frac(uv.x);
    color.g = frac(uv.y);

    // 棋盘格模式 (便于观察重复)
    float2 checker = floor(uv * 10.0);
    float checkerPattern = (checker.x + checker.y) % 2.0;
    color.b = checkerPattern;

    return color;
}

/**
 * @brief 深度可视化
 *
 * @param depth 线性深度
 * @param nearPlane 近平面
 * @param farPlane 远平面
 * @return 可视化颜色
 */
float3 DepthToColor(float depth, float nearPlane, float farPlane) {
    // 将深度映射到0-1
    float t = (depth - nearPlane) / (farPlane - nearPlane);

    if (g_DebugHeatmap) {
        return HeatmapColor(t);
    } else {
        // 灰度，近处亮远处暗
        return float3(t, t, t);
    }
}

/**
 * @brief 标量值可视化
 *
 * @param value 输入值
 * @param minVal 最小值
 * @param maxVal 最大值
 * @return 可视化颜色
 */
float3 ScalarToColor(float value, float minVal, float maxVal) {
    float t = (value - minVal) / (maxVal - minVal);

    if (g_DebugHeatmap) {
        return HeatmapColor(t);
    } else {
        return float3(t, t, t);
    }
}

// ============================================================================
// 调试可视化函数
// ============================================================================

/**
 * @brief 主调试可视化函数
 *
 * @param input 调试输入数据
 * @return 可视化颜色
 */
float3 DebugVisualize(DebugInput input) {
    switch (g_DebugMode) {
        case DEBUG_MODE_NONE:
            return input.albedo;  // 正常渲染

        case DEBUG_MODE_WIREFRAME:
            return float3(0.0, 1.0, 0.0);  // 绿色线框

        case DEBUG_MODE_NORMALS:
            return NormalToColor(input.normalWS);

        case DEBUG_MODE_TANGENTS:
            return NormalToColor(input.tangentWS);

        case DEBUG_MODE_BITANGENTS:
            return NormalToColor(input.bitangentWS);

        case DEBUG_MODE_UV:
            return UVToColor(input.uv);

        case DEBUG_MODE_UV2:
            return UVToColor(input.uv2);

        case DEBUG_MODE_VERTEX_COLORS:
            return input.color.rgb;

        case DEBUG_MODE_DEPTH:
            return DepthToColor(input.depth, 0.0, 1.0);

        case DEBUG_MODE_LINEAR_DEPTH:
            return DepthToColor(input.linearDepth, 0.1, 100.0);

        case DEBUG_MODE_ALBEDO:
            return input.albedo;

        case DEBUG_MODE_METALLIC:
            if (g_DebugHeatmap) {
                return HeatmapColor(input.metallic);
            } else {
                return float3(input.metallic, input.metallic, input.metallic);
            }

        case DEBUG_MODE_ROUGHNESS:
            if (g_DebugHeatmap) {
                return HeatmapColor(input.roughness);
            } else {
                return float3(input.roughness, input.roughness, input.roughness);
            }

        case DEBUG_MODE_AO:
            return float3(input.ao, input.ao, input.ao);

        case DEBUG_MODE_EMISSION:
            return input.emission;

        case DEBUG_MODE_SPECULAR:
            return input.specular;

        case DEBUG_MODE_FACE_NORMALS:
            return NormalToColor(input.faceNormalWS);

        case DEBUG_MODE_MESH_BOUNDS:
        case DEBUG_MODE_BOUNDS:
            return float3(1.0, 0.0, 1.0);  // 洋红色包围盒

        default:
            return float3(1.0, 0.0, 1.0);  // 错误：洋红色
    }
}

/**
 * @brief 是否应该使用调试模式
 */
bool IsDebugMode() {
    return g_DebugMode != DEBUG_MODE_NONE;
}

/**
 * @brief 是否覆盖最终颜色
 *
 * 某些调试模式（如线框）应该与正常渲染混合
 */
bool ShouldOverrideColor() {
    return g_DebugMode != DEBUG_MODE_NONE &&
           g_DebugMode != DEBUG_MODE_EMISSION;
}

#endif // DEBUG_HLSL
