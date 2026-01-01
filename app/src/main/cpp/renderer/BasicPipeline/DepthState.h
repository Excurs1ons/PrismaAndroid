/**
 * @file DepthState.h
 * @brief 深度测试状态配置
 *
 * 控制深度缓冲区的读写行为，实现正确的遮挡关系
 */

#pragma once

/**
 * @brief 深度比较函数
 *
 * 决定片段深度与缓冲区深度的比较方式
 */
enum class DepthCompareFunc {
    /** 永远不通过测试 */
    Never,

    /** 深度小于缓冲区值时通过（片段更近） */
    Less,

    /** 深度等于缓冲区值时通过 */
    Equal,

    /** 深度小于等于缓冲区值时通过 */
    LessEqual,

    /** 深度大于缓冲区值时通过（片段更远） */
    Greater,

    /** 深度不等于缓冲区值时通过 */
    NotEqual,

    /** 深度大于等于缓冲区值时通过 */
    GreaterEqual,

    /** 永远通过测试（不进行深度比较） */
    Always
};

/**
 * @brief 深度状态配置
 *
 * 控制深度测试和深度写入行为
 *
 * 典型使用场景:
 * - 不透明物体: DepthTest=Enable, DepthWrite=Enable, Compare=LessEqual
 * - 透明物体: DepthTest=Enable, DepthWrite=Disable, Compare=LessEqual
 * - 天空盒: DepthTest=Enable, DepthWrite=Disable, Compare=Equal
 * - UI元素: DepthTest=Disable, DepthWrite=Disable
 * - 粒子系统: DepthTest=Enable, DepthWrite=Disable, Compare=Less
 */
struct DepthState {
    // ========================================================================
    // 核心配置
    // ========================================================================

    /**
     * 是否启用深度测试
     *
     * - true: 片段在渲染前会与深度缓冲区比较，决定是否渲染
     * - false: 直接渲染片段，不考虑深度
     */
    bool depthTestEnable = true;

    /**
     * 是否启用深度写入
     *
     * - true: 渲染成功后将片段深度写入深度缓冲区
     * - false: 只读取不写入深度缓冲区
     *
     * 注意: 透明物体通常设为false，避免遮挡后续透明片段
     */
    bool depthWriteEnable = true;

    /**
     * 深度比较函数
     *
     * 决定片段深度值与深度缓冲区的比较方式:
     * - LessEqual (默认): 深度 <= 缓冲区值时通过（标准深度测试）
     * - Less: 深度 < 缓冲区值时通过
     * - Greater: 深度 > 缓冲区值时通过（用于深度反转）
     * - Equal: 深度 == 缓冲区值时通过（用于后处理等）
     * - Always: 永远通过（不进行深度比较）
     */
    DepthCompareFunc depthCompareFunc = DepthCompareFunc::LessEqual;

    /**
     * 是否启用深度边界测试
     *
     * 深度边界测试是一个额外的测试，检查深度值是否在指定范围内
     * 仅在部分API支持（Vulkan）
     */
    bool depthBoundsTestEnable = false;

    /** 深度边界最小值 */
    float minDepthBounds = 0.0f;

    /** 深度边界最大值 */
    float maxDepthBounds = 1.0f;

    // ========================================================================
    // 预设配置
    // ========================================================================

    /**
     * @brief 获取默认的深度状态（用于不透明物体）
     */
    static DepthState defaultOpaque() {
        DepthState state;
        state.depthTestEnable = true;
        state.depthWriteEnable = true;
        state.depthCompareFunc = DepthCompareFunc::LessEqual;
        return state;
    }

    /**
     * @brief 获取透明物体的深度状态
     *
     * 透明物体需要深度测试但不能写入深度，
     * 否则会遮挡后面应该显示的透明片段
     */
    static DepthState transparent() {
        DepthState state;
        state.depthTestEnable = true;
        state.depthWriteEnable = false;  // 不写入深度
        state.depthCompareFunc = DepthCompareFunc::LessEqual;
        return state;
    }

    /**
     * @brief 获取天空盒的深度状态
     *
     * 天空盒只在未渲染任何东西的地方显示
     */
    static DepthState skybox() {
        DepthState state;
        state.depthTestEnable = true;
        state.depthWriteEnable = false;
        state.depthCompareFunc = DepthCompareFunc::Equal;  // 只在深度=1时绘制
        return state;
    }

    /**
     * @brief 获取禁用深度的状态（用于UI等）
     */
    static DepthState disabled() {
        DepthState state;
        state.depthTestEnable = false;
        state.depthWriteEnable = false;
        return state;
    }

    /**
     * @brief 获取只读深度的状态（用于后期遮罩等）
     */
    static DepthState readOnly() {
        DepthState state;
        state.depthTestEnable = true;
        state.depthWriteEnable = false;
        state.depthCompareFunc = DepthCompareFunc::LessEqual;
        return state;
    }

    /**
     * @brief 获取深度反转的状态（用于提高远距离精度）
     *
     * 深度反转技巧: 使用 [1, 0] 范围代替 [0, 1]，提高远距离深度精度
     */
    static DepthState reversed() {
        DepthState state;
        state.depthTestEnable = true;
        state.depthWriteEnable = true;
        state.depthCompareFunc = DepthCompareFunc::Greater;
        return state;
    }

    // ========================================================================
    // 工具方法
    // ========================================================================

    /**
     * @brief 比较两个深度状态是否相同
     */
    bool operator==(const DepthState& other) const {
        return depthTestEnable == other.depthTestEnable &&
               depthWriteEnable == other.depthWriteEnable &&
               depthCompareFunc == other.depthCompareFunc &&
               depthBoundsTestEnable == other.depthBoundsTestEnable &&
               minDepthBounds == other.minDepthBounds &&
               maxDepthBounds == other.maxDepthBounds;
    }

    /**
     * @brief 比较两个深度状态是否不同
     */
    bool operator!=(const DepthState& other) const {
        return !(*this == other);
    }

    /**
     * @brief 获取哈希值（用于状态缓存）
     */
    size_t getHash() const {
        size_t h = 0;
        h ^= (depthTestEnable ? 1 : 0) << 0;
        h ^= (depthWriteEnable ? 1 : 0) << 1;
        h ^= static_cast<size_t>(depthCompareFunc) << 2;
        h ^= (depthBoundsTestEnable ? 1 : 0) << 5;
        return h;
    }
};

/**
 * @brief 深度偏见（Depth Bias）配置
 *
 * 用于解决Z-fighting（深度冲突）问题
 *
 * Z-fighting发生原因:
 * - 两个表面非常接近，深度精度不足区分
 * - 常见于 decals贴花、阴影贴图等
 *
 * 解决方法:
 * - 添加深度偏移量，使物体看起来"更近"或"更远"
 */
struct DepthBiasInfo {
    /** 是否启用深度偏见 */
    bool enable = false;

    /**
     * 深度偏移常量
     *
     * 这是一个不可缩放的偏移量，对所有片段一视同仁
     * 单位取决于深度缓冲区的精度
     */
    float constantFactor = 0.0f;

    /**
     * 深度偏移斜率因子
     *
     * 根据片段的表面斜率动态调整偏移量
     * 斜率越大（越倾斜），偏移量越大
     * 用于处理倾斜表面的深度冲突
     */
    float slopeFactor = 0.0f;

    /**
     * 偏移量的钳位值
     *
     * 限制最大偏移量，避免过度偏移
     */
    float clamp = 0.0f;

    /**
     * @brief 默认构造（禁用）
     */
    static DepthBiasInfo disabled() {
        return DepthBiasInfo{};
    }

    /**
     * @brief 阴影贴图的典型偏移
     */
    static DepthBiasInfo shadowMap() {
        DepthBiasInfo bias;
        bias.enable = true;
        bias.constantFactor = 1.0f;
        bias.slopeFactor = 1.5f;
        return bias;
    }

    /**
     * @brief 贴花（Decal）的典型偏移
     */
    static DepthBiasInfo decal() {
        DepthBiasInfo bias;
        bias.enable = true;
        bias.constantFactor = -0.001f;  // 负值使其"更靠近"相机
        bias.slopeFactor = 0.0f;
        return bias;
    }
};
