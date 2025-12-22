# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.22.1)

# 设置HTTP和HTTPS代理
set(ENV{HTTP_PROXY} "http://127.0.0.1:7897")
set(ENV{HTTPS_PROXY} "http://127.0.0.1:7897")

# ... 你其他的CMake命令，例如 project(), add_executable() 等
project("myapplication")

include(FetchContent)
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
)
FetchContent_MakeAvailable(glm)

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
        Utility.cpp)

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
