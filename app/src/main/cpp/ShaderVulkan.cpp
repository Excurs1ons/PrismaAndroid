//
// Created by jasonngu on 2025/12/22.
//

#include "ShaderVulkan.h"
#include "AndroidOut.h"
#include <vector>

std::vector<uint32_t> ShaderVulkan::loadShader(AAssetManager* assetManager, const std::string& fileName) {
    AAsset* asset = AAssetManager_open(assetManager, fileName.c_str(), AASSET_MODE_BUFFER);
    if (!asset) {
        aout << "Failed to open shader file: " << fileName << std::endl;
        return {};
    }

    size_t size = AAsset_getLength(asset);
    std::vector<uint32_t> buffer(size / sizeof(uint32_t));
    
    AAsset_read(asset, buffer.data(), size);
    AAsset_close(asset);

    return buffer;
}
