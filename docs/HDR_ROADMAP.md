# HDR 功能规划文档

本文档规划两种 HDR 功能的实现：
1. **Bloom 发光效果** - 使用内部 HDR 渲染，最终输出 SDR
2. **真正的 HDR 显示输出** - 输出到 HDR 显示器

---

## 一、Bloom 发光效果

### 概述
在渲染管线内部使用高动态范围（RGB > 1.0），最终通过色调映射输出到 SDR。这是现代游戏引擎的标准做法。

### 渲染流程
```
场景渲染 → HDR Framebuffer → 亮度提取 → 高斯模糊 → 混合 → 色调映射 → SDR 输出
```

### 技术要求

#### 1. HDR 离屏渲染目标
- **格式**: `VK_FORMAT_R16G16B16A16_SFLOAT` (16-bit float)
- **用途**: 存储 HDR 颜色值（可超过 1.0）

#### 2. 亮度提取 (Bright Pass)
- **阈值**: 过滤出亮度 > 阈值（如 1.0）的像素
- **着色器**: `shaders/brightpass.frag`

#### 3. 高斯模糊
- **两路模糊**: 水平 + 垂直方向分离
- **多级模糊**: 可选，实现更好的 Bloom 效果
- **着色器**: `shaders/blur.frag`

#### 4. 色调映射 (Tone Mapping)
- **算法**: ACES Filmic、Reinhard 或曝光调整
- **目的**: 将 HDR 值映射到 [0, 1] SDR 范围
- **着色器**: `shaders/tonemap.frag`

### 实现步骤

#### 阶段 1: 基础架构
- [ ] 创建 `OffscreenRenderPass` 类
- [ ] 添加 HDR Framebuffer 创建
- [ ] 添加浮点纹理创建方法

#### 阶段 2: 亮度提取
- [ ] 创建亮度提取 Pass
- [ ] 实现 `brightpass.frag` 着色器
- [ ] 从 HDR Framebuffer 提取高亮区域

#### 阶段 3: 高斯模糊
- [ ] 实现水平模糊 Pass
- [ ] 实现垂直模糊 Pass
- [ ] 创建 `blur.frag` 着色器
- [ ] 支持多级模糊（可选）

#### 阶段 4: 合成与色调映射
- [ ] 创建后处理 RenderPass
- [ ] 实现 Bloom 混合
- [ ] 实现色调映射算法
- [ ] 添加曝光控制参数

#### 阶段 5: 集成到主渲染器
- [ ] 修改 `RendererVulkan::render()` 流程
- [ ] 添加 Bloom 开关/参数控制
- [ ] 性能优化和调试

### 新增文件

```
app/src/main/cpp/
├── renderer/
│   ├── OffscreenRenderPass.h/cpp    # 离屏渲染 Pass
│   └── BloomRenderer.h/cpp           # Bloom 效果渲染器
└── shaders/
    ├── brightpass.frag               # 亮度提取着色器
    ├── blur.frag                     # 高斯模糊着色器
    └── tonemap.frag                  # 色调映射着色器
```

### 伪代码

```cpp
// 主渲染流程
void RendererVulkan::render() {
    // 1. 渲染场景到 HDR Framebuffer
    vkCmdBeginRenderPass(hdrRenderPass);
    renderScene(hdrFramebuffer);
    vkCmdEndRenderPass();

    // 2. 亮度提取
    brightPass->execute(hdrFramebuffer, brightTexture);

    // 3. 高斯模糊
    blurPass->execute(brightTexture, blurTexture);

    // 4. 合成 + 色调映射到 SwapChain
    tonemapPass->execute(hdrFramebuffer, blurTexture, swapChainImage);
}
```

### 参考资源
- [Learn OpenGL - Bloom](https://learnopengl.com/Advanced-Lighting/Bloom)
- [ACES Filmic Tone Mapping](https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/)

---

## 二、真正的 HDR 显示输出

### 概述
直接输出 HDR 信号到 HDR 显示器，支持高于 1.0 的像素值和更广的色域。

### 技术要求

#### 1. Vulkan 扩展
- **必需**: `VK_EXT_swapchain_colorspace`
- **检查**: `vkEnumeratePhysicalDeviceGroups()`

#### 2. HDR 格式支持

| 格式 | 位深 | 色域 | 用途 |
|------|------|------|------|
| `VK_FORMAT_A2B10G10R10_UNORM_PACK32` | 10-bit | Rec.2020 | HDR10 基础 |
| `VK_FORMAT_R16G16B16A16_SFLOAT` | 16-bit float | Rec.2020 | 高端 HDR |

#### 3. HDR 色彩空间

| 色彩空间 | 枚举值 | 描述 |
|----------|--------|------|
| HDR10 | `VK_COLOR_SPACE_HDR10_ST2084_EXT` | PQ 曲线，10-bit |
| Display P3 | `VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT` | 广色域 SDR |
| HLG | `VK_COLOR_SPACE_HDR10_HLG_EXT` | 混合对数伽马 |

### 实现步骤

#### 阶段 1: 能力检测
- [ ] 检查 `VK_EXT_swapchain_colorspace` 扩展
- [ ] 查询支持的格式和色彩空间
- [ ] 检测显示器 HDR 能力（Android API 26+）

#### 阶段 2: 格式选择
- [ ] 实现 HDR 格式优先级选择
  - 优先: HDR10 PQ
  - 次选: Display P3
  - 回退: SRGB
- [ ] 创建 Surface 格式查询函数

#### 阶段 3: SwapChain 配置
- [ ] 修改 `init()` 中的格式选择
- [ ] 修改 `recreateSwapChain()` 支持动态切换
- [ ] 添加 HDR/SDR 模式切换

#### 阶段 4: 色调映射适配
- [ ] HDR 模式: 保留 > 1.0 值，应用 PQ 转换
- [ ] SDR 模式: 压缩到 [0, 1]

#### 阶段 5: 元数据配置
- [ ] 配置 HDR 元数据（最大亮度、最小亮度）
- [ ] 设置色度坐标

### 代码改动

#### 1. 格式查询函数

```cpp
// VulkanContext.h
struct SwapChainFormatInfo {
    VkFormat format;
    VkColorSpaceKHR colorSpace;
    bool isHDR;
};

class VulkanContext {
    // ...
    SwapChainFormatInfo selectBestFormat();
    bool isHDRSupported();
};
```

#### 2. 格式选择逻辑

```cpp
SwapChainFormatInfo VulkanContext::selectBestFormat() {
    // 查询所有支持的格式
    std::vector<VkSurfaceFormatKHR> formats = querySurfaceFormats();

    // 优先级 1: HDR10 PQ
    for (auto& f : formats) {
        if (f.colorSpace == VK_COLOR_SPACE_HDR10_ST2084_EXT &&
            f.format == VK_FORMAT_A2B10G10R10_UNORM_PACK32) {
            return {f.format, f.colorSpace, true};
        }
    }

    // 优先级 2: Display P3
    for (auto& f : formats) {
        if (f.colorSpace == VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT) {
            return {f.format, f.colorSpace, true};
        }
    }

    // 回退: SRGB SDR
    return {{VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}, false};
}
```

#### 3. PQ 转换着色器

```glsl
// tonemap_hdr.frag
#version 450

layout(location = 0) in vec2 inTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D hdrImage;

// ST2084 PQ 曲线转换
vec3 linearToPQ(vec3 linear) {
    const float m1 = 2610.0 / 4096.0 / 4.0;
    const float m2 = 2523.0 / 4096.0 * 128.0;
    const float c1 = 3424.0 / 4096.0;
    const float c2 = 2413.0 / 4096.0 * 32.0;
    const float c3 = 2392.0 / 4096.0 * 32.0;

    vec3 p = pow(linear, vec3(m1));
    return pow((c1 + c2 * p) / (1.0 + c3 * p), vec3(m2));
}

void main() {
    vec3 hdrColor = texture(hdrImage, inTexCoord).rgb;

    // 应用 PQ 转换
    vec3 pqColor = linearToPQ(hdrColor);

    outColor = vec4(pqColor, 1.0);
}
```

### Android 层面

#### 检测 HDR 能力

```java
// DisplayUtils.java
import android.view.Display;
import android.view.Display.HdrCapabilities;

public class DisplayUtils {
    public static boolean isHDRSupported(Display display) {
        if (android.os.Build.VERSION.SDK_INT >= 26) {
            HdrCapabilities caps = display.getHdrCapabilities();
            return caps != null && caps.getSupportedHdrTypes().length > 0;
        }
        return false;
    }
}
```

### 参考资源
- [VK_EXT_swapchain_colorspace](https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_swapchain_colorspace.html)
- [Android HDR Guide](https://developer.android.com/guide/topics/display/hdr-content)
- [HDR10 Specification](https://cta.tech/Technology-Standards/Standards/HDR10-Plus.aspx)

---

## 三、优先级与时间规划

### 阶段划分

| 阶段 | 功能 | 预计工作量 |
|------|------|-----------|
| 1 | Bloom 发光效果 | 中等 |
| 2 | HDR 显示输出 | 较大 |

### 推荐顺序
1. **先实现 Bloom**: 效果明显，兼容性好，所有设备可用
2. **再实现 HDR 显示**: 需要特定硬件，作为高级特性

---

## 四、注意事项

### 性能考虑
- HDR Framebuffer 带宽是 SDR 的 2 倍
- 高斯模糊是带宽密集型操作
- 考虑提供质量/性能选项

### 兼容性
- Bloom: 所有设备支持
- HDR 显示: 仅 Android 12+ 设备，需要 HDR 显示器
- 必须提供 SDR 回退

### 调试
- 添加 HDR/SDR 切换按钮
- 可视化 Bloom 阈值和强度
- 显示当前色彩空间信息
