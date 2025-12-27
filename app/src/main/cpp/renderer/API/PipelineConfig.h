#ifndef PRISMA_ANDROID_PIPELINE_CONFIG_H
#define PRISMA_ANDROID_PIPELINE_CONFIG_H

#include <string>
#include <vector>
#include "RenderConfig.h"

/**
 * @file PipelineConfig.h
 * @brief 平台无关的 Graphics Pipeline 配置结构
 *
 * 定义创建 Graphics Pipeline 所需的所有配置参数
 * 不包含任何 API 特定的类型
 */

// ============================================================================
// 抽象 Pipeline 对象
// ============================================================================

/**
 * @brief 抽象的 Graphics Pipeline 句柄
 *
 * 封装平台特定的 Pipeline 对象
 * 切换 API 时只需改变底层类型
 */
class GraphicsPipeline {
public:
    virtual ~GraphicsPipeline() = default;

    // 获取原生 Pipeline 对象（仅供内部使用）
    virtual NativePipeline getNative() const = 0;
    virtual NativePipelineLayout getLayout() const = 0;
};

/**
 * @brief 抽象的 Pipeline 工厂接口
 *
 * 平台无关的 Pipeline 创建接口
 * 由具体 API 实现（如 VulkanPipelineFactory）
 */
class PipelineFactory {
public:
    virtual ~PipelineFactory() = default;

    /**
     * 创建 Graphics Pipeline
     * @param config Pipeline 配置
     * @param device 渲染设备
     * @param renderPass 渲染通道
     * @param shaderData 着色器数据（平台特定）
     * @return 创建的 Pipeline 对象
     */
    virtual GraphicsPipeline* createGraphicsPipeline(
        const GraphicsPipelineConfig& config,
        NativeDevice device,
        NativeRenderPass renderPass,
        void* shaderData) = 0;

    /**
     * 销毁 Pipeline
     */
    virtual void destroyPipeline(GraphicsPipeline* pipeline, NativeDevice device) = 0;
};

// ============================================================================
// 枚举配置
// ============================================================================

/**
 * @brief 图元拓扑类型
 */
enum class PrimitiveTopology {
    TriangleList,
    TriangleStrip,
    LineList,
    PointList
};

/**
 * @brief 多边形模式
 */
enum class PolygonMode {
    Fill,
    Line,
    Point
};

/**
 * @brief 剔除模式
 */
enum class CullMode {
    None,
    Front,
    Back,
    FrontAndBack
};

/**
 * @brief 前面方向
 */
enum class FrontFace {
    CounterClockwise,
    Clockwise
};

/**
 * @brief 混合因子
 */
enum class BlendFactor {
    Zero,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
    ConstantColor,
    OneMinusConstantColor,
    SrcAlphaSaturate
};

/**
 * @brief 混合操作
 */
enum class BlendOp {
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max
};

/**
 * @brief 顶点属性格式
 */
enum class VertexFormat {
    Float,
    Float2,
    Float3,
    Float4,
    UInt8,
    UInt8Vec4
};

// ============================================================================
// 配置结构
// ============================================================================

/**
 * @brief 顶点属性描述
 */
struct VertexAttribute {
    uint32_t location;      // 着色器 location
    uint32_t binding;       // 绑定槽
    VertexFormat format;    // 数据格式
    uint32_t offset;        // 偏移量
};

/**
 * @brief 顶点绑定描述
 */
struct VertexBinding {
    uint32_t binding;       // 绑定槽
    uint32_t stride;        // 步长
    bool perInstance;       // 是否按实例（false=按顶点）
};

/**
 * @brief 颜色混合附件配置
 */
struct ColorBlendAttachment {
    bool blendEnable = false;

    // 颜色混合
    BlendFactor srcColorBlendFactor = BlendFactor::One;
    BlendFactor dstColorBlendFactor = BlendFactor::Zero;
    BlendOp colorBlendOp = BlendOp::Add;

    // Alpha 混合
    BlendFactor srcAlphaBlendFactor = BlendFactor::One;
    BlendFactor dstAlphaBlendFactor = BlendFactor::Zero;
    BlendOp alphaBlendOp = BlendOp::Add;

    // 颜色写入掩码 (R=1, G=2, B=4, A=8)
    uint32_t colorWriteMask = 0xF;  // 默认全部写入
};

/**
 * @brief 图形管线配置
 */
struct GraphicsPipelineConfig {
    // ========== 着色器 ==========
    std::string vertexShaderPath;
    std::string fragmentShaderPath;

    // ========== 顶点输入 ==========
    std::vector<VertexBinding> vertexBindings;
    std::vector<VertexAttribute> vertexAttributes;

    // ========== 图元拓扑 ==========
    PrimitiveTopology topology = PrimitiveTopology::TriangleList;
    bool primitiveRestartEnable = false;

    // ========== 视口和裁剪 ==========
    //（动态设置，不包含在配置中）

    // ========== 光栅化 ==========
    bool depthClampEnable = false;
    bool rasterizerDiscardEnable = false;
    PolygonMode polygonMode = PolygonMode::Fill;
    CullMode cullMode = CullMode::Back;
    FrontFace frontFace = FrontFace::CounterClockwise;
    float lineWidth = 1.0f;
    bool depthBiasEnable = false;

    // ========== 多重采样 ==========
    bool sampleShadingEnable = false;
    uint32_t rasterizationSamples = 1;  // 1, 2, 4, 8, 16, 32, 64

    // ========== 深度模板 ==========
    bool depthTestEnable = true;
    bool depthWriteEnable = true;
    bool depthBoundsTestEnable = false;
    bool stencilTestEnable = false;

    // ========== 颜色混合 ==========
    bool logicOpEnable = false;
    ColorBlendAttachment blendAttachment;

    // ========== 描述符布局 ==========
    RenderDescriptorLayout descriptorSetLayout = nullptr;

    // ========== 渲染通道 ==========
    // (动态设置，不包含在配置中)
};

// ============================================================================
// Vulkan 转换辅助函数（仅在 Vulkan 实现中使用）
// ============================================================================

#if RENDER_API_VULKAN

#include <vulkan/vulkan.h>

inline VkPrimitiveTopology vulkanTopology(PrimitiveTopology topology) {
    switch (topology) {
        case PrimitiveTopology::TriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case PrimitiveTopology::TriangleStrip: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case PrimitiveTopology::LineList: return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case PrimitiveTopology::PointList: return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    }
    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

inline VkPolygonMode vulkanPolygonMode(PolygonMode mode) {
    switch (mode) {
        case PolygonMode::Fill: return VK_POLYGON_MODE_FILL;
        case PolygonMode::Line: return VK_POLYGON_MODE_LINE;
        case PolygonMode::Point: return VK_POLYGON_MODE_POINT;
    }
    return VK_POLYGON_MODE_FILL;
}

inline VkCullModeFlags vulkanCullMode(CullMode mode) {
    switch (mode) {
        case CullMode::None: return 0;
        case CullMode::Front: return VK_CULL_MODE_FRONT_BIT;
        case CullMode::Back: return VK_CULL_MODE_BACK_BIT;
        case CullMode::FrontAndBack: return VK_CULL_MODE_FRONT_AND_BACK;
    }
    return 0;
}

inline VkFrontFace vulkanFrontFace(FrontFace face) {
    return (face == FrontFace::Clockwise)
        ? VK_FRONT_FACE_CLOCKWISE
        : VK_FRONT_FACE_COUNTER_CLOCKWISE;
}

inline VkBlendFactor vulkanBlendFactor(BlendFactor factor) {
    switch (factor) {
        case BlendFactor::Zero: return VK_BLEND_FACTOR_ZERO;
        case BlendFactor::One: return VK_BLEND_FACTOR_ONE;
        case BlendFactor::SrcColor: return VK_BLEND_FACTOR_SRC_COLOR;
        case BlendFactor::OneMinusSrcColor: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case BlendFactor::DstColor: return VK_BLEND_FACTOR_DST_COLOR;
        case BlendFactor::OneMinusDstColor: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case BlendFactor::SrcAlpha: return VK_BLEND_FACTOR_SRC_ALPHA;
        case BlendFactor::OneMinusSrcAlpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case BlendFactor::DstAlpha: return VK_BLEND_FACTOR_DST_ALPHA;
        case BlendFactor::OneMinusDstAlpha: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case BlendFactor::ConstantColor: return VK_BLEND_FACTOR_CONSTANT_COLOR;
        case BlendFactor::OneMinusConstantColor: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
        case BlendFactor::SrcAlphaSaturate: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
    }
    return VK_BLEND_FACTOR_ZERO;
}

inline VkBlendOp vulkanBlendOp(BlendOp op) {
    switch (op) {
        case BlendOp::Add: return VK_BLEND_OP_ADD;
        case BlendOp::Subtract: return VK_BLEND_OP_SUBTRACT;
        case BlendOp::ReverseSubtract: return VK_BLEND_OP_REVERSE_SUBTRACT;
        case BlendOp::Min: return VK_BLEND_OP_MIN;
        case BlendOp::Max: return VK_BLEND_OP_MAX;
    }
    return VK_BLEND_OP_ADD;
}

inline VkFormat vulkanVertexFormat(VertexFormat format) {
    switch (format) {
        case VertexFormat::Float: return VK_FORMAT_R32_SFLOAT;
        case VertexFormat::Float2: return VK_FORMAT_R32G32_SFLOAT;
        case VertexFormat::Float3: return VK_FORMAT_R32G32B32_SFLOAT;
        case VertexFormat::Float4: return VK_FORMAT_R32G32B32A32_SFLOAT;
        case VertexFormat::UInt8: return VK_FORMAT_R8_UINT;
        case VertexFormat::UInt8Vec4: return VK_FORMAT_R8G8B8A8_UINT;
    }
    return VK_FORMAT_R32G32B32_SFLOAT;
}

#endif // RENDER_API_VULKAN

#endif //PRISMA_ANDROID_PIPELINE_CONFIG_H
