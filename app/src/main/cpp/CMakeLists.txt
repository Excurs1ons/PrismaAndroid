# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.22.1)
# 设置HTTP和HTTPS代理
# set(ENV{HTTP_PROXY} "http://127.0.0.1:7897")
# set(ENV{HTTPS_PROXY} "http://127.0.0.1:7897")
# 本地开发时如果需要代理，请通过环境变量 HTTP_PROXY 和 HTTPS_PROXY 设置
# 不要在 CMakeLists.txt 中硬编码代理地址，这会导致 CI 环境构建失败
project("myapplication")

include(FetchContent)

message(STATUS "Fetching PrismaEngine...")
FetchContent_Declare(
        PrismaEngine
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../../../src/engine
)
# 让 CMake 把它加进来（会执行 MyEngine 里的 CMakeLists.txt）
FetchContent_MakeAvailable(PrismaEngine)
message(STATUS "PrismaEngine fetched successfully!")

message(STATUS "Fetching GLM...")
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
)
# 关键步骤：在调用 MakeAvailable 之前，禁用 GLM 的测试和示例
# GLM_BUILD_TESTS: 关闭单元测试
# GLM_BUILD_TESTS: 关闭示例代码
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
message(STATUS "Setting Build Test Off")
set(GLM_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
message(STATUS "Setting Build Example Off")
FetchContent_MakeAvailable(glm)
message(STATUS "GLM fetched successfully!")

# ==========================================
# 1. 引入 Snappy (Google的压缩库)
# ==========================================
message(STATUS "Fetching Snappy...")

# 优化编译：关掉 Snappy 的测试和 benchmark，我们不需要编译这些耗时的东西
set(SNAPPY_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SNAPPY_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE) # 游戏引擎通常推荐静态链接

FetchContent_Declare(
        snappy
        GIT_REPOSITORY https://github.com/google/snappy.git
        GIT_TAG main # 建议锁定一个稳定版本号，不要用 master
)
FetchContent_MakeAvailable(snappy)

message(STATUS "Snappy fetched successfully!")

# ==========================================
# 2. 引入 Hap Decoder
# ==========================================
message(STATUS "Fetching Hap Decoder...")

FetchContent_Declare(
        hap_decoder
        GIT_REPOSITORY https://github.com/Vidvox/hap.git
)

# 注意：Hap 官方仓库没有 CMakeLists.txt
# 所以我们只用 FetchContent 下载源码，然后手动定义 Library
FetchContent_GetProperties(hap_decoder)
if(NOT hap_decoder_POPULATED)
    FetchContent_Populate(hap_decoder)

    # 手动创建一个静态库目标
    # 注意：hap.c 文件在 source 子目录中
    add_library(hap_static STATIC
            ${hap_decoder_SOURCE_DIR}/source/hap.c
    )

    # 关键点：Hap 的源码里有一行 #include "snappy-c.h"
    # 我们需要把 Snappy 的头文件路径暴露给 Hap
    target_include_directories(hap_static PRIVATE
            ${snappy_SOURCE_DIR}
            ${hap_decoder_SOURCE_DIR}/source
    )
    target_link_libraries(hap_static PRIVATE snappy)

    # 把 Hap 的头文件路径暴露给你的引擎
    target_include_directories(hap_static PUBLIC ${hap_decoder_SOURCE_DIR}/source)
endif()

message(STATUS "Hap Decoder fetched successfully!")


# 创建一个名为 stb 的接口库
# INTERFACE 意味着它不产生编译产物（如 .lib 或 .a），只传递属性
add_library(stb INTERFACE)

# 添加头文件搜索路径
# 这里的 ${CMAKE_CURRENT_SOURCE_DIR} 指的是 CMakeLists.txt 所在的目录
target_include_directories(stb INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/external/stb
)


# 创建myapplication.so
# The name must be the same as the
# one used for loading in your Kotlin/Java or AndroidManifest.txt files.
add_library(myapplication SHARED
        main.cpp
        AndroidOut.cpp
        Renderer.cpp
        RendererOpenGL.cpp
        RendererVulkan.cpp
        ShaderOpenGL.cpp
        ShaderVulkan.cpp
        VulkanContext.cpp
        TextureAsset.cpp
        Utility.cpp
        CubemapTextureAsset.cpp
        SkyboxRenderer.cpp
        # 逻辑渲染层
        renderer/RenderPipeline.cpp
        renderer/OpaquePass.cpp
        renderer/BackgroundPass.cpp
        stb_impl.cpp
)

# Searches for a package provided by the game activity dependency
find_package(game-activity REQUIRED CONFIG)

# Searches for Vulkan library
find_library(vulkan-lib vulkan)

# Configure libraries CMake uses to link your target library.
target_link_libraries(myapplication
        # The game activity
        game-activity::game-activity

        # EGL and other dependent libraries required for drawing
        # and interacting with Android system
        EGL
        GLESv3
        jnigraphics
        android
        log
        ${vulkan-lib}
        # 链接 stb
        # 链接 stb 后，自动继承 external/stb 这个 include 路径
        stb
)

target_link_options(myapplication PRIVATE "-Wl,-z,max-page-size=16384")

target_link_libraries(myapplication glm
        hap_static  # 手动定义的 Hap 库
        snappy      # Snappy 库
)
