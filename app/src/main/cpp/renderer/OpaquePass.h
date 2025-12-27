#ifndef PRISMA_ANDROID_OPAQUE_PASS_H
#define PRISMA_ANDROID_OPAQUE_PASS_H
#include "../Model.h"
#include "RenderPass.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

// 前向声明
struct android_app;

class MeshRenderer;
class GameObject;
struct Scene;

// === 数据结构定义（暂时保留，后续会移动到各 Pass 中） ===

struct RenderObjectData {
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;
    std::vector<VkDescriptorSet> descriptorSets;
};

/**
 * 不透明物体渲染通道
 *
 * 渲染所有不透明的 3D 物体（MeshRenderer）
 *
 * 特点：
 * - 支持纹理映射
 * - 支持透明混合（src_alpha, one_minus_src_alpha）
 * - 双面渲染（无剔除）
 *
 * 等价关系：
 * OpaquePass ≈ graphicsPipeline + RenderObjectData[] + 渲染命令
 *
 * API 切换注意事项：
 * - descriptorSetLayout_ 是 Vulkan 特有的 Descriptor Set 概念
 *   DirectX 12 使用 Root Signature，Metal 使用 Argument Buffer
 * - renderObjects_ 中的 VkBuffer 等资源需要替换为对应 API 的类型
 */
class OpaquePass : public RenderPass {
public:

    OpaquePass():RenderPass("Opaque Pass"){};
    ~OpaquePass() override = default;

    // 禁止拷贝
    OpaquePass(const OpaquePass&) = delete;
    OpaquePass& operator=(const OpaquePass&) = delete;

    // 允许移动
    OpaquePass(OpaquePass&&) = default;
    OpaquePass& operator=(OpaquePass&&) = default;

    /**
     * 初始化不透明物体渲染通道
     * 创建 graphics pipeline
     */
    void initialize(VkDevice device, VkRenderPass renderPass) override;

    /**
     * 记录渲染命令
     * 渲染所有添加的渲染对象
     */
    void record(VkCommandBuffer cmdBuffer) override;

    /**
     * 清理资源
     */
    void cleanup(VkDevice device) override;

    // === 数据设置方法 ===

    /**
     * 添加渲染对象（转移所有权）
     * @param object 渲染对象数据
     */
    void addRenderObject(const RenderObjectData& object);

    /**
     * 设置当前帧索引（用于选择正确的 uniform buffer）
     * @param currentFrame 当前帧索引
     */
    void setCurrentFrame(uint32_t currentFrame);

    /**
     * 设置 descriptor set layout [Vulkan-Specific]
     */
    void setDescriptorSetLayout(VkDescriptorSetLayout layout);

    /**
     * 设置 SwapChain 扩展信息（用于 viewport/scissor）
     */
    void setSwapChainExtent(VkExtent2D extent);

    /**
     * 设置 android_app（用于加载着色器）
     */
    void setAndroidApp(android_app* app);

    /**
     * 设置 Scene（用于渲染时获取游戏对象）
     */
    void setScene(Scene* scene);

    /**
     * 更新 Uniform Buffer
     * @param gameObjects 游戏对象列表
     * @param view 视图矩阵
     * @param proj 投影矩阵
     * @param currentImage 当前帧索引
     * @param time 当前时间（用于动画）
     */
    void updateUniformBuffer(const std::vector<std::shared_ptr<GameObject>>& gameObjects,
                             const Matrix4& view, const Matrix4& proj,
                             uint32_t currentImage, float time);

private:
    /**
     * 创建 graphics pipeline
     * 从 RendererVulkan::createGraphicsPipeline() 迁移
     */
    void createPipeline(VkDevice device, VkRenderPass renderPass);

    // === 数据成员 ===

    std::vector<RenderObjectData> renderObjects_;  // 持有的渲染对象数据

    // [Vulkan-Specific] Descriptor Set 布局，切换 API 时需替换
    VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
    // DirectX 12: Root Signature
    // Metal: Argument Buffer

    uint32_t currentFrame_ = 0;  // 当前帧索引

    // 配置信息
    VkExtent2D swapChainExtent_{};  // SwapChain 扩展
    android_app* app_ = nullptr;    // android_app（用于加载着色器）
    Scene* scene_ = nullptr;        // Scene（用于渲染时获取游戏对象）
};

#endif //PRISMA_ANDROID_OPAQUE_PASS_H
