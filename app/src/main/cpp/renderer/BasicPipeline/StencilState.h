/**
 * @file StencilState.h
 * @brief 模板测试状态配置
 *
 * 模板测试用于:
 * - 镜面反射渲染（只在镜面区域渲染反射）
 * - 端口传送效果
 * - 遮罩效果
 * - 轮廓渲染
 * - 体积渲染的边界提取
 */

#pragma once

/**
 * @brief 模板操作
 *
 * 指定在测试失败/通过后如何修改模板缓冲区
 */
enum class StencilOp {
    /** 保持原值（不修改） */
    Keep = 0,

    /** 设置为0 */
    Zero = 1,

    /** 设置为参考值 */
    Replace = 2,

    /** 递增（饱和，最大值保持不变） */
    IncrementAndClamp = 3,

    /** 递减（饱和，最小值保持为0） */
    DecrementAndClamp = 4,

    /** 递增（循环，超过最大值回到0） */
    Invert = 5,

    /** 递减（循环，小于0回到最大值） */
    IncrementAndWrap = 6,

    /** 按位取反 */
    DecrementAndWrap = 7
};

/**
 * @brief 模板比较函数
 */
enum class StencilCompareFunc {
    /** 永远不通过测试 */
    Never = 0,

    /** 片段模板值 < 参考值 时通过 */
    Less = 1,

    /** 片段模板值 == 参考值 时通过 */
    Equal = 2,

    /** 片段模板值 <= 参考值 时通过 */
    LessEqual = 3,

    /** 片段模板值 > 参考值 时通过 */
    Greater = 4,

    /** 片段模板值 != 参考值 时通过 */
    NotEqual = 5,

    /** 片段模板值 >= 参考值 时通过 */
    GreaterEqual = 6,

    /** 永远通过测试 */
    Always = 7
};

/**
 * @brief 单面的模板状态配置
 *
 * 模板测试可以分别对正面和背面设置不同的行为
 */
struct StencilFaceState {
    // ========================================================================
    // 核心配置
    // ========================================================================

    /**
     * 模板比较函数
     *
     * 决定片段模板值与参考值的比较方式
     */
    StencilCompareFunc compareFunc = StencilCompareFunc::Always;

    /**
     * 比较操作使用的参考值
     *
     * 在比较时与模板缓冲区值进行比较
     */
    uint32_t reference = 0;

    /**
     * 比较掩码
     *
     * 在比较前，对参考值和模板缓冲区值进行 AND 操作
     * 只有对应的位才会参与比较
     *
     * 示例: mask = 0xFF 表示所有8位都参与比较
     */
    uint32_t compareMask = 0xFFFFFFFF;

    /**
     * 写入掩码
     *
     * 在写入模板缓冲区前，对写入值进行 AND 操作
     * 控制哪些位可以被修改
     */
    uint32_t writeMask = 0xFFFFFFFF;

    /**
     * 模板测试失败时的操作
     *
     * 当片段未通过模板比较时执行
     */
    StencilOp failOp = StencilOp::Keep;

    /**
     * 模板测试通过但深度测试失败时的操作
     *
     * 当片段通过模板比较但未通过深度测试时执行
     */
    StencilOp depthFailOp = StencilOp::Keep;

    /**
     * 模板测试和深度测试都通过时的操作
     *
     * 当片段通过所有测试时执行
     */
    StencilOp passOp = StencilOp::Keep;

    // ========================================================================
    // 工具方法
    // ========================================================================

    /**
     * @brief 创建默认的禁用状态
     */
    static StencilFaceState disabled() {
        StencilFaceState state;
        state.compareFunc = StencilCompareFunc::Always;
        state.failOp = StencilOp::Keep;
        state.depthFailOp = StencilOp::Keep;
        state.passOp = StencilOp::Keep;
        state.compareMask = 0xFFFFFFFF;
        state.writeMask = 0xFFFFFFFF;
        return state;
    }

    /**
     * @brief 比较两个状态是否相同
     */
    bool operator==(const StencilFaceState& other) const {
        return compareFunc == other.compareFunc &&
               reference == other.reference &&
               compareMask == other.compareMask &&
               writeMask == other.writeMask &&
               failOp == other.failOp &&
               depthFailOp == other.depthFailOp &&
               passOp == other.passOp;
    }

    bool operator!=(const StencilFaceState& other) const {
        return !(*this == other);
    }
};

/**
 * @brief 完整的模板状态配置
 *
 * 包含正面和背面的模板状态
 */
struct StencilState {
    // ========================================================================
    // 核心配置
    // ========================================================================

    /** 是否启用模板测试 */
    bool enable = false;

    /** 正面模板状态 */
    StencilFaceState front;

    /** 背面模板状态 */
    StencilFaceState back;

    // ========================================================================
    // 预设配置
    // ========================================================================

    /**
     * @brief 获取禁用的模板状态（默认）
     */
    static StencilState disabled() {
        StencilState state;
        state.enable = false;
        state.front = StencilFaceState::disabled();
        state.back = StencilFaceState::disabled();
        return state;
    }

    /**
     * @brief 创建镜面反射的模板写入状态
     *
     * 使用方法:
     * 1. 渲染镜面物体: 使用此状态，将镜面区域标记为参考值
     * 2. 渲染反射场景: 使用 maskEqual(reference)，只在镜面区域渲染
     */
    static StencilState mirrorWrite(uint32_t reference = 1) {
        StencilState state;
        state.enable = true;

        // 正面和背面都设置相同的操作
        StencilFaceState face;
        face.compareFunc = StencilCompareFunc::Always;
        face.reference = reference;
        face.failOp = StencilOp::Keep;
        face.depthFailOp = StencilOp::Keep;
        face.passOp = StencilOp::Replace;  // 通过后替换为参考值

        state.front = face;
        state.back = face;
        return state;
    }

    /**
     * @brief 创建镜面反射的遮罩状态
     *
     * 只在模板值等于参考值的区域渲染
     */
    static StencilState mirrorMask(uint32_t reference = 1) {
        StencilState state;
        state.enable = true;

        StencilFaceState face;
        face.compareFunc = StencilCompareFunc::Equal;
        face.reference = reference;
        face.failOp = StencilOp::Keep;
        face.depthFailOp = StencilOp::Keep;
        face.passOp = StencilOp::Keep;  // 保持模板值不变

        state.front = face;
        state.back = face;
        return state;
    }

    /**
     * @brief 创建轮廓渲染的内缩状态（第一遍）
     *
     * 轮廓渲染步骤:
     * 1. 渲染物体，正常写入模板
     * 2. 渲染放大的物体，使用此状态
     * 3. 渲染正常物体，只在模板值未改变的区域渲染
     */
    static StencilState outlineInlay(uint32_t reference = 1) {
        StencilState state;
        state.enable = true;

        StencilFaceState face;
        face.compareFunc = StencilCompareFunc::Always;
        face.reference = reference;
        face.passOp = StencilOp::IncrementAndClamp;  // 递增模板值

        state.front = face;
        state.back = face;
        return state;
    }

    /**
     * @brief 创建体积渲染的边界状态
     *
     * 用于渲染只在边缘显示的效果（如辉光边缘）
     */
    static StencilState volumeBoundary() {
        StencilState state;
        state.enable = true;

        // 正面: 递增
        StencilFaceState frontFace;
        frontFace.compareFunc = StencilCompareFunc::Always;
        frontFace.passOp = StencilOp::IncrementAndClamp;

        // 背面: 递减
        StencilFaceState backFace;
        backFace.compareFunc = StencilCompareFunc::Always;
        backFace.passOp = StencilOp::DecrementAndClamp;

        state.front = frontFace;
        state.back = backFace;
        return state;
    }

    /**
     * @brief 创建端口传送的遮罩写入状态
     *
     * 用于传送门效果
     */
    static StencilState portalWrite(uint32_t reference = 1) {
        return mirrorWrite(reference);  // 与镜面相同
    }

    /**
     * @brief 创建端口传送的遮罩状态
     */
    static StencilState portalMask(uint32_t reference = 1) {
        return mirrorMask(reference);  // 与镜面相同
    }

    // ========================================================================
    // 工具方法
    // ========================================================================

    /**
     * @brief 比较两个状态是否相同
     */
    bool operator==(const StencilState& other) const {
        return enable == other.enable &&
               front == other.front &&
               back == other.back;
    }

    bool operator!=(const StencilState& other) const {
        return !(*this == other);
    }

    /**
     * @brief 获取哈希值（用于状态缓存）
     */
    size_t getHash() const {
        size_t h = enable ? 1 : 0;
        h ^= static_cast<size_t>(front.compareFunc) << 1;
        h ^= static_cast<size_t>(back.compareFunc) << 4;
        h ^= static_cast<size_t>(front.reference) << 7;
        h ^= static_cast<size_t>(back.reference) << 10;
        return h;
    }
};

/**
 * @brief 模板缓冲区格式
 */
enum class StencilFormat {
    /** 无模板缓冲区 */
    None = 0,

    /** 8位模板值（0-255） */
    S8 = 1,

    /** 24位深度 + 8位模板 */
    D24S8 = 2,

    /** 32位深度（8位用于模板） */
    D32S8 = 3
};
