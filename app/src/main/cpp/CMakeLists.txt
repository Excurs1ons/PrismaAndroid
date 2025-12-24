# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.22.1)

# 本地开发时如果需要代理，请通过环境变量 HTTP_PROXY 和 HTTPS_PROXY 设置
# 不要在 CMakeLists.txt 中硬编码代理地址，这会导致 CI 环境构建失败
project("myapplication")

include(FetchContent)

message(STATUS "Fetching GLM...")
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
)
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
        # 建议锁定一个稳定版本号，不要用 master
)
FetchContent_MakeAvailable(snappy)

message(STATUS "Snappy fetched successfully!")


# Creates your game shared library. The name must be the same as the
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
        renderer/BackgroundPass.cpp)

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
        ${vulkan-lib})
target_link_options(myapplication PRIVATE "-Wl,-z,max-page-size=16384")

target_link_libraries(myapplication glm)
