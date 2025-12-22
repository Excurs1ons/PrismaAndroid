//
// Created by jasonngu on 2025/12/22.
//

#ifndef MY_APPLICATION_SHADERVULKAN_H
#define MY_APPLICATION_SHADERVULKAN_H

#include <vector>
#include <string>
#include <vulkan/vulkan.h>
#include <android/asset_manager.h>

class ShaderVulkan {
public:
    static std::vector<uint32_t> loadShader(AAssetManager* assetManager, const std::string& fileName);
};

#endif //MY_APPLICATION_SHADERVULKAN_H
