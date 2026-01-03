# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.22.1)
# 设置HTTP和HTTPS代理
# set(ENV{HTTP_PROXY} "http://127.0.0.1:7897")
# set(ENV{HTTPS_PROXY} "http://127.0.0.1:7897")
# 本地开发时如果需要代理，请通过环境变量 HTTP_PROXY 和 HTTPS_PROXY 设置
# 不要在 CMakeLists.txt 中硬编码代理地址，这会导致 CI 环境构建失败
project("myapplication")

# ========== Android 平台配置 ==========

# 启用 C++20 标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置 Android 平台特定选项
set(ANDROID TRUE)

# ========== 选项配置 (需要在 FetchContent 之前) ==========

# 禁用 Editor (Android 不支持)
set(PRISMA_BUILD_EDITOR OFF CACHE BOOL "Build Editor application" FORCE)

# 启用 Vulkan 渲染 (Android 主要渲染后端)
set(PRISMA_ENABLE_RENDER_VULKAN ON CACHE BOOL "Enable Vulkan render device" FORCE)

# 禁用 DirectX12 (Android 不支持)
set(PRISMA_ENABLE_RENDER_DX12 OFF CACHE BOOL "Enable DirectX12 render device" FORCE)

# 可选：启用 OpenGL 作为备用渲染后端
set(PRISMA_ENABLE_RENDER_OPENGL OFF CACHE BOOL "Enable OpenGL render device" FORCE)

# 禁用 SDL3 音频和渲染（Android 不需要 SDL3）
set(PRISMA_ENABLE_AUDIO_SDL3 OFF CACHE BOOL "Enable SDL3 audio device" FORCE)
set(PRISMA_ENABLE_RENDER_SDL3 OFF CACHE BOOL "Enable SDL3 render device" FORCE)

# 使用 GLM (Android 平台)
set(PRISMA_USE_DIRECTXMATH OFF CACHE BOOL "Use DirectXMath" FORCE)
set(PRISMA_FORCE_GLM ON CACHE BOOL "Force use GLM" FORCE)

# 启用 FetchContent 管理依赖
set(PRISMA_USE_FETCHCONTENT ON CACHE BOOL "Use FetchContent" FORCE)

# ========== 包含引擎的 CMake 模块 ==========

# 获取 PrismaEngine 根目录
# 从 app/src/main/cpp 向上 7 级到 PrismaEngine 根目录
get_filename_component(PRISMA_ENGINE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../" ABSOLUTE)

# 打印路径用于调试
message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "PRISMA_ENGINE_ROOT: ${PRISMA_ENGINE_ROOT}")
message(STATUS "DeviceOptions.cmake exists: ${EXISTS}${PRISMA_ENGINE_ROOT}/cmake/DeviceOptions.cmake}")
message(STATUS "FetchContent.cmake exists: ${EXISTS}${PRISMA_ENGINE_ROOT}/cmake/FetchContent.cmake}")

# 包含引擎的 CMake 模块
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${PRISMA_ENGINE_ROOT}/cmake")

# 包含设备选项配置
include(${PRISMA_ENGINE_ROOT}/cmake/DeviceOptions.cmake OPTIONAL)

# 包含 FetchContent 依赖管理 (会自动下载所有需要的依赖)
include(${PRISMA_ENGINE_ROOT}/cmake/FetchThirdPartyDeps.cmake)

# ========== 添加引擎子目录 ==========

message(STATUS "=== 添加 PrismaEngine 源码 ===")

# 引擎源码已经在本地，直接添加子目录
set(ENGINE_SOURCE_DIR ${PRISMA_ENGINE_ROOT}/src/engine)
set(ENGINE_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/engine)

message(STATUS "Engine source: ${ENGINE_SOURCE_DIR}")
message(STATUS "Engine binary: ${ENGINE_BINARY_DIR}")

# 添加引擎作为子项目
add_subdirectory(${ENGINE_SOURCE_DIR} ${ENGINE_BINARY_DIR})


message(STATUS "PrismaEngine 已添加为子项目")

# ========== 添加运行时子目录 ==========
# 引擎源码已经在本地，直接添加子目录
set(RUNTIME_SOURCE_DIR ${PRISMA_ENGINE_ROOT}/src/runtime/android/)

message(STATUS "Runtime source: ${RUNTIME_SOURCE_DIR}")

# ========== 创建应用库 ==========

# 创建 myapplication.so
add_library(myapplication SHARED
        # Runtime 文件（使用完整路径）
        ${RUNTIME_SOURCE_DIR}/AndroidRuntime.cpp

        # 本地 Android 特定文件
        # ${RUNTIME_SOURCE_DIR}/AndroidOut.cpp
        ${RUNTIME_SOURCE_DIR}/Renderer.cpp
        ${RUNTIME_SOURCE_DIR}/RendererOpenGL.cpp
        ${RUNTIME_SOURCE_DIR}/RendererVulkan.cpp
        ${RUNTIME_SOURCE_DIR}/ShaderOpenGL.cpp
        ${RUNTIME_SOURCE_DIR}/ShaderVulkan.cpp
        ${RUNTIME_SOURCE_DIR}/VulkanContext.cpp
        ${RUNTIME_SOURCE_DIR}/LightComponent.cpp
        ${RUNTIME_SOURCE_DIR}/AndroidInputBackend.cpp
        # ${RUNTIME_SOURCE_DIR}/TextureAsset.cpp
        ${RUNTIME_SOURCE_DIR}/Utility.cpp
        # ${RUNTIME_SOURCE_DIR}/CubemapTextureAsset.cpp
        ${RUNTIME_SOURCE_DIR}/SkyboxRenderer.cpp
        # 逻辑渲染层
        ${RUNTIME_SOURCE_DIR}/renderer/RenderPipeline.cpp
        ${RUNTIME_SOURCE_DIR}/renderer/OpaquePass.cpp
        ${RUNTIME_SOURCE_DIR}/renderer/BackgroundPass.cpp
        ${RUNTIME_SOURCE_DIR}/renderer/UIPass.cpp
        ${RUNTIME_SOURCE_DIR}/stb_impl.cpp
)

# ========== 链接依赖 ==========

# 查找 game-activity 包
find_package(game-activity REQUIRED CONFIG)

# 查找 Vulkan 库
find_library(vulkan-lib vulkan)

# 链接应用依赖
target_link_libraries(myapplication
        # 游戏 activity
        game-activity::game-activity

        # Android 系统库
        EGL
        GLESv3
        jnigraphics
        android
        log
        ${vulkan-lib}

        # 引擎库 (已经包含了所有 FetchContent 的依赖和 Android 适配器)
        Engine
)

# ========== STB 接口库 ==========

# 创建 STB 接口库
#add_library(stb_interface INTERFACE)
#target_include_directories(stb_interface INTERFACE
#        ${CMAKE_CURRENT_SOURCE_DIR}/external/stb
#)

add_library(runtime INTERFACE)
target_include_directories(runtime INTERFACE
        ${PRISMA_ENGINE_ROOT}/src/runtime/android
        ${PRISMA_ENGINE_ROOT}/src/runtime/android/renderer
        ${PRISMA_ENGINE_ROOT}/src/runtime/android/renderer/API
)
# 链接
target_link_libraries(myapplication
        # stb_interface
        runtime
)

# ========== 链接选项 ==========

target_link_options(myapplication PRIVATE "-Wl,-z,max-page-size=16384")
