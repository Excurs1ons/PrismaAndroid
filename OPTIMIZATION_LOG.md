# 项目极致精简与优化记录

本文档记录了对项目进行的精简、资源剔除和代码剥离操作。

## 1. 构建配置优化 (已自动应用)

修改了 `app/build.gradle.kts` 文件：

- **ABI 过滤**: 配置项目仅构建和打包 `arm64-v8a` 架构的库。
  - 在 `android.defaultConfig.ndk` 中添加了 `abiFilters.add("arm64-v8a")`。
  - 在 `android.defaultConfig.externalNativeBuild.cmake` 中添加了 `abiFilters("arm64-v8a")`。

- **代码与资源压缩**: 在 Release 构建中启用了混淆和资源缩减。
  - 设置 `isMinifyEnabled = true`。
  - 设置 `isShrinkResources = true`。

## 2. C++ 代码剥离 (已自动应用)

移除了未使用的 OpenGL 渲染后端代码：

- **Renderer.cpp**: 移除了对 `RendererOpenGL` 的引用和实例化逻辑，强制使用 Vulkan 后端。
- **CMakeLists.txt**:
  - 从编译列表中移除了 `RendererOpenGL.cpp` 和 `ShaderOpenGL.cpp`。
  - 从链接库列表中移除了 `EGL` 和 `GLESv3`。

## 3. 资源清理建议 (需手动执行)

由于系统权限限制，以下文件/目录未被自动删除，建议手动删除以进一步减小项目体积：

### 未使用的图片资源
保留 `mipmap-xxxhdpi` 作为高分辨率图标源，删除以下低分辨率目录：
- `app/src/main/res/mipmap-mdpi/`
- `app/src/main/res/mipmap-hdpi/`
- `app/src/main/res/mipmap-xhdpi/`
- `app/src/main/res/mipmap-xxhdpi/`

### 未使用的夜间模式资源
如果不需要特定的夜间模式样式：
- `app/src/main/res/values-night/`

### 未使用的源代码文件
以下文件已从构建系统中移除，可以物理删除：
- `app/src/main/cpp/RendererOpenGL.h`
- `app/src/main/cpp/RendererOpenGL.cpp`
- `app/src/main/cpp/ShaderOpenGL.h`
- `app/src/main/cpp/ShaderOpenGL.cpp`

## 4. 验证

构建 Release 版本 APK 并检查其大小和包含的库架构：
```bash
./gradlew assembleRelease
```
解压 APK 确认 `lib/` 目录下仅包含 `arm64-v8a`。
