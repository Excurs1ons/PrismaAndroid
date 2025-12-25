# 渲染架构文档

本文档描述 PrismaAndroid 的渲染架构设计。

**核心思想**：逻辑架构是对 Vulkan API 的封装，二者是等价的。逻辑层提供更清晰的代码组织，为未来切换 RenderAPI 做准备。

---

## 一、架构概览

### 1.1 概念对照

| 逻辑概念 | Vulkan API 对应 | 说明 |
|----------|----------------|------|
| **逻辑 RenderPipeline** | 无直接对应，封装整个渲染流程 | 管理多个渲染通道，定义执行顺序 |
| **逻辑 RenderPass** | VkPipeline（部分）+ 渲染命令 | 封装一种渲染类型的所有逻辑 |
| **Vulkan VkRenderPass** | Vulkan API | 描述渲染流程中的附件、子通道和依赖关系 |
| **Vulkan VkPipeline** | Vulkan API | 描述渲染状态（着色器、混合、深度测试等） |

### 1.2 架构图

```
┌─────────────────────────────────────────────────────────────────┐
│                    逻辑架构（Vulkan API 封装）                    │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  RenderPipeline (逻辑)                                    │  │
│  │  - 等价于：VkRenderPass + 内部的多个 VkPipeline          │  │
│  │  - 管理 Pass 的执行顺序                                   │  │
│  │  - 提供统一的初始化/执行/清理接口                         │  │
│  │                                                           │  │
│  │  ┌────────────────────────────────────────────────────┐  │  │
│  │  │  Vulkan VkRenderPass（API 层）                      │  │
│  │  │  vkCmdBeginRenderPass() ──────────────────┐         │  │  │
│  │  │                                          │         │  │  │
│  │  │  ┌────────────────────────────────────┐  │         │  │  │
│  │  │  │  BackgroundPass (逻辑)             │  │         │  │  │
│  │  │  │  等价于：skyboxPipeline +          │  │         │  │  │
│  │  │  │         clearColorPipeline +       │  │         │  │  │
│  │  │  │         渲染命令                    │  │         │  │  │
│  │  │  └────────────────────────────────────┘  │         │  │  │
│  │  │                                          │         │  │  │
│  │  │  ┌────────────────────────────────────┐  │         │  │  │
│  │  │  │  OpaquePass (逻辑)                 │  │         │  │  │
│  │  │  │  等价于：graphicsPipeline +        │  │         │  │  │
│  │  │  │         渲染命令                    │  │         │  │  │
│  │  │  └────────────────────────────────────┘  │         │  │  │
│  │  │                                          │         │  │  │
│  │  │  vkCmdEndRenderPass() ◄─────────────────┘         │  │  │
│  │  └────────────────────────────────────────────────────┘  │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 1.3 等价关系说明

**逻辑 RenderPipeline ≈ Vulkan 渲染流程**
```
RenderPipeline::execute()
    等价于：
    vkCmdBeginRenderPass()
    + 执行多个 VkPipeline 的渲染命令
    + vkCmdEndRenderPass()
```

**逻辑 RenderPass ≈ VkPipeline + 数据 + 命令录制**
```
BackgroundPass
    等价于：
    skyboxPipeline + clearColorPipeline
    + SkyboxRenderData + ClearColorData
    + vkCmdBindPipeline() + vkCmdDraw*() 命令

OpaquePass
    等价于：
    graphicsPipeline
    + RenderObjectData[]
    + vkCmdBindPipeline() + vkCmdDraw*() 命令
```

---

## 二、逻辑架构

### 2.1 类层次结构

```
RenderPipeline (逻辑 Pipeline)
    │
    ├── 管理多个 RenderPass
    ├── 定义执行顺序
    └── 提供 initialize() / execute() / cleanup() 接口
            │
            ▼
    RenderPass (逻辑 Pass 基类)
    │
    ├── initialize() - 创建渲染资源
    ├── record() - 记录渲染命令
    └── cleanup() - 清理资源
            │
            ├── BackgroundPass
            │   ├── 渲染 Skybox（cubemap）
            │   └── 渲染纯色背景（备用）
            │
            └── OpaquePass
                └── 渲染不透明 3D 物体
```

### 2.2 数据流动

```
Scene (场景数据)
    │
    ├── MeshRenderer[] → OpaquePass::renderObjects_
    │
    └── SkyboxRenderer → BackgroundPass::skyboxData_
        └── (无纹理) → BackgroundPass::clearColorData_
```

### 2.3 执行流程

```
RendererVulkan::init()
    │
    ├── 创建 Vulkan 资源
    │   ├── VkRenderPass
    │   ├── Descriptor Pool
    │   └── Uniform Buffers
    │
    └── 创建 RenderPipeline
        │
        ├── 创建 BackgroundPass
        │   ├── setSkyboxData()
        │   └── addPass()
        │
        ├── 创建 OpaquePass
        │   ├── addRenderObject() (循环添加所有对象)
        │   └── addPass()
        │
        └── initialize() → 初始化所有 Pass

RendererVulkan::render()
    │
    └── recordCommandBuffer()
        │
        ├── vkCmdBeginRenderPass()
        │
        ├── renderPipeline_->execute()
        │   ├── BackgroundPass::record() (先渲染背景)
        │   └── OpaquePass::record() (再渲染物体)
        │
        └── vkCmdEndRenderPass()
```

---

## 三、Vulkan API 映射

### 3.1 VkRenderPass 在代码中的位置

```cpp
// RendererVulkan.cpp:297
VkRenderPassCreateInfo renderPassInfo{};
renderPassInfo.attachmentCount = 1;
renderPassInfo.pAttachments = &colorAttachment;
renderPassInfo.subpassCount = 1;
renderPassInfo.pSubpasses = &subpass;
vkCreateRenderPass(vulkanContext_.device, &renderPassInfo, nullptr, &vulkanContext_.renderPass);
```

**VkRenderPass 定义了：**
- **颜色附件**：渲染到哪里（SwapChain 的图像）
- **Subpass**：渲染阶段的划分（当前只有 1 个）
- **依赖关系**：Subpass 之间的内存依赖

### 3.2 VkPipeline 在代码中的分布

| 逻辑 Pass | 创建的 VkPipeline | 用途 |
|-----------|------------------|------|
| BackgroundPass | `skyboxPipeline` | 渲染天空盒（cubemap 纹理） |
| BackgroundPass | `clearColorPipeline` | 渲染纯色背景 |
| OpaquePass | `graphicsPipeline` | 渲染普通 3D 物体 |

### 3.3 命令录制流程

```cpp
// RendererVulkan.cpp:1462
vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

// === BackgroundPass ===
vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipeline);
vkCmdBindDescriptorSets(...);
vkCmdBindVertexBuffers(...);
vkCmdBindIndexBuffers(...);
vkCmdDrawIndexed(...);

// === OpaquePass ===
vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
for (每个渲染对象) {
    vkCmdBindDescriptorSets(...);
    vkCmdBindVertexBuffers(...);
    vkCmdBindIndexBuffers(...);
    vkCmdDrawIndexed(...);
}

vkCmdEndRenderPass(commandBuffer);
```

---

## 四、API 切换指南

### 4.1 抽象层次

```
┌─────────────────────────────────────────────────────────┐
│                   游戏逻辑层                             │
│            (Scene, GameObject, Component)                │
└─────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│                逻辑渲染层 (API 无关)                      │
│        (RenderPipeline, RenderPass, OpaquePass)          │
└─────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────┐
│                  渲染 API 抽象层                          │
│           (RendererVulkan, RendererD3D12, ...)           │
└─────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────┬───────────────┬─────────────────────────┐
│   Vulkan        │  DirectX 12   │      Metal              │
│                 │               │                         │
│ - VkRenderPass  │ - Root Signature │ - MTLRender...      │
│ - VkPipeline    │ - PSO         │ - MTLPipelineState     │
│ - VkCommandBuffer│ - CommandList │ - MTLCommandEncoder   │
└─────────────────┴───────────────┴─────────────────────────┘
```

### 4.2 切换 API 需要修改的部分

| 组件 | Vulkan | DirectX 12 | Metal |
|------|--------|------------|-------|
| RenderPass::initialize() | VkDevice, VkRenderPass | ID3D12Device, 无需参数 | MTLDevice |
| RenderPass::record() | VkCommandBuffer | ID3D12GraphicsCommandList | MTLRenderCommandEncoder |
| RenderPass::pipeline_ | VkPipeline | ID3D12PipelineState | MTLRenderPipelineState |
| RenderPass::pipelineLayout_ | VkPipelineLayout | Root Signature | MTLArgumentEncoder |
| Descriptor Set | VkDescriptorSet | Descriptor Heap | Argument Buffer |

---

## 五、文件组织

```
app/src/main/cpp/
├── renderer/                    # 逻辑渲染层（API 无关）
│   ├── RenderPipeline.h         # 逻辑 Pipeline（Pass 容器）
│   ├── RenderPass.h             # 逻辑 Pass 基类
│   ├── OpaquePass.h             # 不透明物体 Pass
│   └── BackgroundPass.h         # 背景 Pass
│
├── RendererVulkan.h             # Vulkan 渲染器实现
├── RendererVulkan.cpp           # Vulkan 特定代码
└── VulkanContext.h              # Vulkan 上下文
```

---

## 六、未来扩展

### 6.1 添加新的 Pass

```cpp
// Bloom Pass（后处理）
class BloomPass : public RenderPass {
    // 输入：HDR 纹理
    // 输出：模糊后的纹理
};

// Tone Mapping Pass（色调映射）
class ToneMapPass : public RenderPass {
    // 输入：HDR 纹理 + Bloom 纹理
    // 输出：SDR 图像
};
```

### 6.2 多 Pass 渲染流程

```cpp
// HDR + Bloom 渲染流程
RenderPipeline hdrPipeline;
hdrPipeline.addPass(std::make_unique<OpaquePass>());        // 场景 → HDR Framebuffer
hdrPipeline.addPass(std::make_unique<BrightPass>());        // 亮度提取
hdrPipeline.addPass(std::make_unique<BlurPass>());          // 高斯模糊
hdrPipeline.addPass(std::make_unique<ToneMapPass>());       // 色调映射 → SwapChain
```

---

## 七、总结

1. **逻辑 RenderPipeline** 管理多个 **逻辑 RenderPass**
2. 每个 **逻辑 RenderPass** 持有自己的数据和 **Vulkan VkPipeline**
3. **Vulkan VkRenderPass** 是 API 层的概念，包含整个渲染流程
4. 这种抽象设计为切换 RenderAPI 提供了清晰的接口边界
