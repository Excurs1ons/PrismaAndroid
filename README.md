# PrismaAndroid

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/platform-Android%20(ARM64)-blue.svg)](https://github.com/Excurs1ons/PrismaAndroid)
[![Vulkan](https://img.shields.io/badge/Vulkan-1.1%2B-success.svg)](https://vulkan-tutorial.com/)

PrismaAndroid 是一个功能完整的 Android Vulkan 渲染运行时，为 [PrismaEngine](https://github.com/Excurs1ons/PrismaEngine) 提供跨平台渲染能力。

简体中文 | [English](#english)

## 项目概述

PrismaAndroid 是 PrismaEngine 的 Android Vulkan 运行时参考实现，包含约 **3300 行** 功能完整的 C++ 代码，演示了如何在 Android 平台上使用 Vulkan API 构建高性能渲染器。

### 核心功能

- ✅ **VulkanContext** (~300 行) - Instance/Device/SwapChain 管理
- ✅ **RendererVulkan** (~1900 行) - 完整的 Vulkan 渲染管线
- ✅ **ShaderVulkan** - SPIR-V 着色器加载
- ✅ **TextureAsset** - 纹理加载与 Mipmap 生成
- ✅ **屏幕旋转支持** - SwapChain 重建机制
- ✅ **Flight Frame 同步** - 多帧缓冲管理

### 技术栈

| 组件 | 说明 |
|------|------|
| **渲染 API** | Vulkan 1.1+ |
| **着色器** | SPIR-V |
| **数学库** | GLM |
| **架构** | ARM64 (arm64-v8a) |
| **最低 SDK** | API 30 (Android 11) |
| **目标 SDK** | API 34 (Android 14) |

## 项目结构

```
PrismaAndroid/
├── app/src/main/cpp/
│   ├── VulkanContext.{h,cpp}       # Vulkan 上下文管理
│   ├── RendererVulkan.{h,cpp}      # Vulkan 渲染器
│   ├── ShaderVulkan.{h,cpp}        # SPIR-V 着色器
│   ├── TextureAsset.{h,cpp}        # 纹理资源
│   ├── GameObject.h                # 简化 ECS 架构
│   ├── Component.h
│   ├── MeshRenderer.h
│   └── main.cpp                    # 入口
├── app/src/main/java/              # Java Activity
└── app/src/main/res/               # 资源文件
```

## VulkanContext 功能

`VulkanContext` 提供 Vulkan 初始化和辅助功能：

```cpp
class VulkanContext {
public:
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapChain;
    // ...

    // 缓冲区管理
    void createBuffer(size, usage, properties, buffer, bufferMemory);
    void copyBuffer(srcBuffer, dstBuffer, size);

    // 图像操作
    void transitionImageLayout(image, format, oldLayout, newLayout, mipLevels);
    void copyBufferToImage(buffer, image, width, height);
    void generateMipmaps(image, format, width, height, mipLevels);

    // 内存管理
    uint32_t findMemoryType(typeFilter, properties);
};
```

## RendererVulkan 功能

`RendererVulkan` 实现完整的渲染循环：

- ✅ 图形管线创建 (Pipeline/RenderPass/Framebuffers)
- ✅ 描述符池和描述符集管理
- ✅ Uniform Buffer 每帧更新
- ✅ 顶点/索引缓冲区管理
- ✅ 纹理加载和采样
- ✅ **屏幕旋转支持** (SwapChain 重建)
- ✅ 飞行帧 (Flight Frame) 同步机制

## 快速开始

### 前置要求

- Android Studio Hedgehog | 2023.1.1 或更高版本
- Android SDK API 34
- Android NDK (ARM64)
- CMake 3.22.1 或更高版本
- 支持 Vulkan 1.1+ 的 Android 设备或模拟器

### 构建命令

```bash
# Debug 构建
./gradlew assembleDebug

# Release 构建
./gradlew assembleRelease

# 安装 Debug APK
./gradlew installDebug

# 清理构建
./gradlew clean
```

## 与 PrismaEngine 的关系

PrismaAndroid 的代码正在逐步迁移到 PrismaEngine：

| PrismaAndroid | PrismaEngine | 状态 |
|---------------|--------------|------|
| `VulkanContext` | `graphic/vulkan/VulkanContext` | ⏳ 计划迁移 |
| `RendererVulkan` | `graphic/vulkan/VulkanRenderer` | ⏳ 计划迁移 |
| `ShaderVulkan` | `graphic/vulkan/VulkanShader` | ⏳ 计划迁移 |
| `TextureAsset` | `graphic/vulkan/VulkanTexture` | ⏳ 计划迁移 |
| `GameObject/Component` | 已有 ECS | ✅ 使用引擎架构 |

详细迁移计划: [PrismaEngine - Android 平台集成](https://github.com/Excurs1ons/PrismaEngine/blob/main/docs/VulkanIntegration.md)

## 代码统计

| 文件 | 行数 | 说明 |
|------|------|------|
| RendererVulkan.cpp | ~1900 | 核心渲染逻辑 |
| TextureAsset.cpp | ~300 | 纹理管理 |
| VulkanContext.cpp | ~320 | 上下文管理 |
| 其他 | ~750 | 工具类、OpenGL 后端等 |
| **总计** | **~3300** | - |

## 依赖库

- [Vulkan-Headers](https://github.com/KhronosGroup/Vulkan-Headers) - Vulkan API 头文件
- [GLM](https://github.com/g-truc/glm) - OpenGL 数学库
- [STB](https://github.com/nothings/stb) - 图像加载 (stb_image)

## 参考资料

- [Vulkan Tutorial](https://vulkan-tutorial.com/)
- [Vulkan Guide](https://vulkan-guide.com/)
- [Android NDK Vulkan Guide](https://developer.android.com/ndk/guides/graphics/vulkan)

## 许可证

MIT License - 详见 [LICENSE](LICENSE)

---

---

# English

## Overview

PrismaAndroid is a fully-functional Android Vulkan rendering runtime for [PrismaEngine](https://github.com/Excurs1ons/PrismaEngine), containing approximately **3300 lines** of C++ code.

### Core Features

- ✅ **VulkanContext** (~300 lines) - Instance/Device/SwapChain management
- ✅ **RendererVulkan** (~1900 lines) - Complete Vulkan rendering pipeline
- ✅ **ShaderVulkan** - SPIR-V shader loading
- ✅ **TextureAsset** - Texture loading with Mipmap generation
- ✅ **Screen rotation support** - SwapChain recreation

### Relation to PrismaEngine

PrismaAndroid code is being migrated to PrismaEngine. See [Android Integration Plan](https://github.com/Excurs1ons/PrismaEngine/blob/main/docs/VulkanIntegration.md) for details.
