/**
 * @file PBRCommon.hlsl
 * @brief PBR光照模型通用定义和常量
 *
 * 基于GGX/Cook-Torrance BRDF的物理渲染
 * 参考: Disney BRDF, UE4, Unity URP PBR
 */

#ifndef PBR_COMMON_HLSL
#define PBR_COMMON_HLSL

// ============================================================================
// 光照特性开关（用于Shader变体和调试）
// ============================================================================

/**
 * 这些宏定义在编译时确定，通过shader变体系统切换
 * 运行时可通过uniform变量控制开关
 */

// 直接光照开关
#ifndef ENABLE_DIRECT_LIGHTING
#define ENABLE_DIRECT_LIGHTING 1
#endif

// 间接光照开关
#ifndef ENABLE_INDIRECT_LIGHTING
#define ENABLE_INDIRECT_LIGHTING 0  // TODO: 暂未实现
#endif

// 全局反射开关
#ifndef ENABLE_GLOBAL_REFLECTION
#define ENABLE_GLOBAL_REFLECTION 0  // TODO: 暂未实现
#endif

// 全局阴影开关
#ifndef ENABLE_GLOBAL_SHADOW
#define ENABLE_GLOBAL_SHADOW 1
#endif

// 环境光遮蔽开关
#ifndef ENABLE_AO
#define ENABLE_AO 0
#endif

// 发光开关
#ifndef ENABLE_EMISSION
#define ENABLE_EMISSION 1
#endif

// ============================================================================

// ============================================================================
// 数学常量
// ============================================================================

#define PI 3.14159265359
#define TWO_PI 6.28318530718
#define INV_PI 0.31830988618
#define HALF_PI 1.57079632679

// ============================================================================

// ============================================================================
// 光照模型结构
// ============================================================================

/**
 * @brief 材质属性
 */
struct MaterialAttributes {
    // 基础颜色（反照率）
    float3 albedo;

    // 金属度 (0.0 = 介电质, 1.0 = 金属)
    float metallic;

    // 粗糙度 (0.0 = 光滑, 1.0 = 粗糙)
    float roughness;

    // 表面反射率（用于非金属）
    float3 specular;

    // 发光颜色
    float3 emission;

    // 环境光遮蔽 (0.0 = 全遮蔽, 1.0 = 无遮蔽)
    float occlusion;

    // Alpha裁剪阈值
    float alphaClipThreshold;

    // 是否使用Alpha测试
    bool useAlphaClip;
};

/**
 * @brief 光照输入数据
 */
struct LightInput {
    // 光源方向（指向光源）
    float3 direction;

    // 光源颜色（已包含强度）
    float3 color;

    // 衰减距离（点光源/聚光灯）
    float distanceAttenuation;

    // 聚光灯角度衰减（聚光灯）
    float spotAttenuation;

    // 阴影衰减
    float shadowAttenuation;
};

/**
 * @brief BRDF参数
 */
struct BRDFData {
    float3 diffuse;      // 漫反射反射率
    float3 specular;     // 镜面反射率
    float perceptualRoughness;  // 感知粗糙度
    float roughness;     // 线性粗糙度
    float roughness2;    // 粗糙度平方
    float normalizationTerm;    // 镜面反射归一化项
    float3 grazingTerm;         // 菲涅尔掠射项
};

// ============================================================================
// 函数声明
// ============================================================================

/**
 * @brief 初始化BRDF数据
 *
 * @param material 材质属性
 * @return BRDF数据
 */
BRDFData InitializeBRDFData(MaterialAttributes material);

/**
 * @brief GGX分布函数 (Trowbridge-Reitz)
 *
 * 描述微表面法线分布
 *
 * @param N 表面法线
 * @param H 半程向量
 * @param roughness 粗糙度
 * @return 分布值 D(N, H)
 */
float DistributionGGX(float3 N, float3 H, float roughness);

/**
 * @brief 几何遮蔽函数 (Smith)
 *
 * 描述微表面自遮挡
 *
 * @param N 表面法线
 * @param V 视线方向
 * @param L 光照方向
 * @param roughness 粗糙度
 * @return 几何因子 G(N, V, L)
 */
float GeometrySmith(float3 N, float3 V, float3 L, float roughness);

/**
 * @brief 单向几何遮蔽函数 (Schlick-GGX)
 *
 * @param N 表面法线
 * @param V 视线方向或光照方向
 * @param roughness 粗糙度
 * @return 几何因子
 */
float GeometrySchlickGGX(float3 N, float3 V, float roughness);

/**
 * @brief 菲涅尔反射函数 (Schlick近似)
 *
 * @param F0 入射角为0度的反射率
 * @param V 视线方向
 * @param H 半程向量
 * @return 菲涅尔反射率 F(V, H)
 */
float3 FresnelSchlick(float3 F0, float3 V, float3 H);

/**
 * @brief 带粗糙度的菲涅尔反射函数
 *
 * 用于环境光和间接光
 *
 * @param F0 入射角为0度的反射率
 * @param V 视线方向
 * @param roughness 粗糙度
 * @return 菲涅尔反射率
 */
float3 FresnelSchlickRoughness(float3 F0, float3 V, float roughness);

/**
 * @brief Cook-Torrance BRDF
 *
 * 镜面反射BRDF = D * F * G / (4 * (N·L) * (N·V))
 *
 * @param N 表面法线
 * @param V 视线方向
 * @param L 光照方向
 * @param brdfData BRDF数据
 * @return 镜面反射颜色
 */
float3 SpecularBRDF(float3 N, float3 V, float3 L, BRDFData brdfData);

/**
 * @brief Lambert漫反射BRDF
 *
 * 漫反射BRDF = albedo / PI
 *
 * @param brdfData BRDF数据
 * @return 漫反射颜色
 */
float3 DiffuseBRDF(BRDFData brdfData);

/**
 * @brief 计算直接光照
 *
 * @param lightInput 光照输入
 * @param N 表面法线
 * @param V 视线方向
 * @param brdfData BRDF数据
 * @return 光照颜色
 */
float3 CalculateDirectLighting(LightInput lightInput, float3 N, float3 V, BRDFData brdfData);

/**
 * @brief 计算间接光照（环境光）
 *
 * TODO: 实现光照探针和光照贴图
 *
 * @param N 表面法线
 * @param V 视线方向
 * @param brdfData BRDF数据
 * @param material 材质属性
 * @return 间接光颜色
 */
float3 CalculateIndirectLighting(float3 N, float3 V, BRDFData brdfData, MaterialAttributes material);

/**
 * @brief 计算全局反射（屏幕空间反射或反射探针）
 *
 * TODO: 实现SSR或反射探针
 *
 * @param N 表面法线
 * @param V 视线方向
 * @param roughness 粗糙度
 * @param positionWS 世界空间位置
 * @return 反射颜色
 */
float3 CalculateGlobalReflection(float3 N, float3 V, float roughness, float3 positionWS);

/**
 * @brief 计算阴影衰减
 *
 * TODO: 实现阴影贴图采样
 *
 * @param positionWS 世界空间位置
 * @param lightIndex 光源索引
 * @return 阴影衰减 (0.0 = 全阴影, 1.0 = 无阴影)
 */
float CalculateShadowAttenuation(float3 positionWS, int lightIndex);

// ============================================================================
// 实现（在include此文件后可用）
// ============================================================================

BRDFData InitializeBRDFData(MaterialAttributes material) {
    BRDFData data;

    // 计算感知粗糙度
    data.perceptualRoughness = material.roughness;

    // 计算线性粗糙度
    data.roughness = data.perceptualRoughness * data.perceptualRoughness;

    // 粗糙度平方（用于优化）
    data.roughness2 = data.roughness * data.roughness;

    // 计算菲涅尔F0（0度入射角的反射率）
    float3 F0 = lerp(material.specular, material.albedo, material.metallic);

    // 漫反射反射率（金属没有漫反射）
    data.diffuse = lerp(material.albedo, (float3)0.0, material.metallic);

    // 镜面反射反射率
    data.specular = F0;

    // 归一化项
    float roughness2 = data.roughness2;
    float normalizationTerm = data.roughness / (roughness2 + 1.0) * (4.0 / PI) + 0.0001;
    data.normalizationTerm = normalizationTerm;

    // 掠射项
    float3 reflectance = max(max(data.specular.r, data.specular.g), data.specular.b);
    float grazingTerm = saturate((1.0 - data.perceptualRoughness) + reflectance);
    data.grazingTerm = (float3)grazingTerm;

    return data;
}

float DistributionGGX(float3 N, float3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

float GeometrySchlickGGX(float3 N, float3 V, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness) {
    float ggx2 = GeometrySchlickGGX(N, V, roughness);
    float ggx1 = GeometrySchlickGGX(N, L, roughness);

    return ggx1 * ggx2;
}

float3 FresnelSchlick(float3 F0, float3 V, float3 H) {
    float cosTheta = max(dot(V, H), 0.0);
    float x = 1.0 - cosTheta;
    float x5 = x * x;
    x5 = x5 * x5 * x;

    return F0 + (1.0 - F0) * x5;
}

float3 FresnelSchlickRoughness(float3 F0, float3 V, float roughness) {
    float cosTheta = max(dot(V, float3(0, 0, 1)), 0.0);
    float x = 1.0 - cosTheta;
    float x5 = x * x;
    x5 = x5 * x5 * x;

    return F0 + (max((float3)(1.0 - roughness), F0) - F0) * x5;
}

float3 SpecularBRDF(float3 N, float3 V, float3 L, BRDFData brdfData) {
    float3 H = normalize(V + L);

    // GGX分布
    float D = DistributionGGX(N, H, brdfData.roughness);

    // Smith几何遮蔽
    float G = GeometrySmith(N, V, L, brdfData.roughness);

    // Schlick菲涅尔
    float3 F = FresnelSchlick(brdfData.specular, V, H);

    // Cook-Torrance BRDF
    float3 numerator = D * F * G;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    float3 specular = numerator / denominator;

    return specular * brdfData.normalizationTerm;
}

float3 DiffuseBRDF(BRDFData brdfData) {
    return brdfData.diffuse * INV_PI;
}

float3 CalculateDirectLighting(LightInput lightInput, float3 N, float3 V, BRDFData brdfData) {
#if ENABLE_DIRECT_LIGHTING
    float3 L = lightInput.direction;
    float3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    if (NdotL <= 0.0 || NdotV <= 0.0) {
        return (float3)0.0;
    }

    // 计算BRDF
    float3 diffuse = DiffuseBRDF(brdfData);
    float3 specular = SpecularBRDF(N, V, L, brdfData);

    // 组合漫反射和镜面反射
    float3 brdf = diffuse + specular;

    // 应用光照颜色和衰减
    float3 lightColor = lightInput.color;
    float attenuation = lightInput.distanceAttenuation * lightInput.spotAttenuation;

#if ENABLE_GLOBAL_SHADOW
    attenuation *= lightInput.shadowAttenuation;
#endif

    return brdf * lightColor * attenuation * NdotL;
#else
    return (float3)0.0;
#endif
}

float3 CalculateIndirectLighting(float3 N, float3 V, BRDFData brdfData, MaterialAttributes material) {
#if ENABLE_INDIRECT_LIGHTING
    // TODO: 实现光照探针、光照贴图
    // 暂时使用简单的环境光
    return (float3)0.0;
#else
    return (float3)0.0;
#endif
}

float3 CalculateGlobalReflection(float3 N, float3 V, float roughness, float3 positionWS) {
#if ENABLE_GLOBAL_REFLECTION
    // TODO: 实现屏幕空间反射或反射探针
    return (float3)0.0;
#else
    return (float3)0.0;
#endif
}

float CalculateShadowAttenuation(float3 positionWS, int lightIndex) {
#if ENABLE_GLOBAL_SHADOW
    // TODO: 实现阴影贴图采样
    return 1.0;  // 暂无阴影
#else
    return 1.0;
#endif
}

#endif // PBR_COMMON_HLSL
