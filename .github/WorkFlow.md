# GitHub Actions 工作流说明

本项目使用 GitHub Actions 进行自动化构建，包含两个工作流：

## 工作流文件

### 1. `build.yml` - 主构建流程

**触发条件：**
- 推送到 `main` 或 `develop` 分支
- 创建 Pull Request
- 手动触发（workflow_dispatch）

**功能：**
- 构建 Debug 和 Release APK
- 自动缓存 NDK、Gradle、CMake 构建文件
- 上传构建产物（保留 30 天）
- 运行 Lint 代码检查

### 2. `release.yml` - 发布流程

**触发条件：**
- 推送格式为 `v*` 的 Git Tag（如 `v1.0.0`）
- 手动触发

**功能：**
- 构建 Release APK
- 创建 GitHub Release
- 自动附加 APK 到 Release 页面

## 缓存策略

为了加快构建速度，配置了三层缓存：

| 缓存类型 | 路径 | 用途 | 预计节省时间 |
|---------|------|------|-------------|
| Gradle | `~/.gradle/caches`, `~/.gradle/wrapper` | Gradle 依赖包 | 1-2 分钟 |
| NDK | `~/Android/Sdk/ndk` | Android NDK（约 1-2GB） | 3-5 分钟 |
| CMake | `**/.cxx` | C++ 构建缓存 | 1-3 分钟 |

**总缓存效果：** 首次构建约 10-15 分钟，后续构建约 2-5 分钟

## 使用方法

### 普通构建
```bash
git push origin main
```

### 发布新版本
```bash
# 创建并推送 Tag
git tag v1.0.0
git push origin v1.0.0
```

### 手动触发
1. 进入 GitHub 仓库页面
2. 点击 "Actions" 标签
3. 选择 "Android CI" 或 "Release Build"
4. 点击 "Run workflow"

## 配置 APK 签名（可选）

如需发布签名的 Release APK，需要在 GitHub Secrets 中添加：

| Secret 名称 | 说明 |
|------------|------|
| `KEYSTORE_FILE` | Base64 编码的 keystore 文件 |
| `KEYSTORE_PASSWORD` | Keystore 密码 |
| `KEY_ALIAS` | 密钥别名 |
| `KEY_PASSWORD` | 密钥密码 |

**生成 Base64 编码的 keystore：**
```bash
base64 -i release.keystore | pbcopy  # macOS
base64 -w 0 release.keystore          # Linux
```

## 下载构建产物

### 从 Actions 页面下载
1. 进入 GitHub Actions 页面
2. 点击对应的工作流运行
3. 在 "Artifacts" 部分下载 APK

### 从 Release 页面下载
Tag 触发的构建会自动创建 Release，APK 可在 Release 页面下载

## 本地构建

```bash
# Debug 版本
./gradlew assembleDebug

# Release 版本
./gradlew assembleRelease

# 安装到设备
./gradlew installDebug
```

## 环境要求

- Java 17
- Android SDK
- Android NDK
- CMake

本地环境配置推荐使用 Android Studio。
