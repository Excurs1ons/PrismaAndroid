#include "CubemapTextureAsset.h"
#include "VulkanContext.h"
#include "AndroidOut.h"
#include <android/asset_manager.h>
#include <android/imagedecoder.h>
#include <vector>

CubemapTextureAsset::CubemapTextureAsset(VulkanContext* context) : TextureAsset(context) {}

CubemapTextureAsset::~CubemapTextureAsset() {
    // 父类Unload会处理清理
}

std::shared_ptr<CubemapTextureAsset> CubemapTextureAsset::loadFromAssets(
        AAssetManager* assetManager,
        const std::vector<std::string>& facePaths,
        VulkanContext* vulkanContext) {

    if (facePaths.size() != 6) {
        aout << "Error: Cubemap requires exactly 6 face paths!" << std::endl;
        return nullptr;
    }

    auto cubemap = std::shared_ptr<CubemapTextureAsset>(new CubemapTextureAsset(vulkanContext));

    // 加载所有6个面的图像数据
    struct FaceData {
        std::vector<uint8_t> pixels;
        int width;
        int height;
    };
    std::vector<FaceData> faceData(6);

    uint32_t faceSize = 0;

    for (int i = 0; i < 6; i++) {
        // 使用AImageDecoder加载图像（与TextureAsset相同的方式）
        auto asset = AAssetManager_open(assetManager, facePaths[i].c_str(), AASSET_MODE_BUFFER);
        if (!asset) {
            aout << "Error: Failed to load cubemap face: " << facePaths[i] << std::endl;
            return nullptr;
        }

        AImageDecoder* decoder = nullptr;
        auto result = AImageDecoder_createFromAAsset(asset, &decoder);
        if (result != ANDROID_IMAGE_DECODER_SUCCESS) {
            AAsset_close(asset);
            aout << "Error: Failed to create decoder for: " << facePaths[i] << std::endl;
            return nullptr;
        }

        AImageDecoder_setAndroidBitmapFormat(decoder, ANDROID_BITMAP_FORMAT_RGBA_8888);

        const AImageDecoderHeaderInfo* header = AImageDecoder_getHeaderInfo(decoder);
        int width = static_cast<int>(AImageDecoderHeaderInfo_getWidth(header));
        int height = static_cast<int>(AImageDecoderHeaderInfo_getHeight(header));
        int stride = AImageDecoder_getMinimumStride(decoder);

        if (i == 0) {
            faceSize = width;
            if (width != height) {
                aout << "Error: Cubemap faces must be square!" << std::endl;
                AImageDecoder_delete(decoder);
                AAsset_close(asset);
                return nullptr;
            }
        } else {
            if (width != static_cast<int>(faceSize) || height != static_cast<int>(faceSize)) {
                aout << "Error: All cubemap faces must have the same size!" << std::endl;
                AImageDecoder_delete(decoder);
                AAsset_close(asset);
                return nullptr;
            }
        }

        faceData[i].width = width;
        faceData[i].height = height;
        faceData[i].pixels.resize(height * stride);

        auto decodeResult = AImageDecoder_decodeImage(
            decoder,
            faceData[i].pixels.data(),
            stride,
            faceData[i].pixels.size());

        AImageDecoder_delete(decoder);
        AAsset_close(asset);

        if (decodeResult != ANDROID_IMAGE_DECODER_SUCCESS) {
            aout << "Error: Failed to decode cubemap face: " << facePaths[i] << std::endl;
            return nullptr;
        }
    }

    // 设置cubemap的尺寸（使用父类的size_成员）
    cubemap->size_ = glm::uvec2(faceSize, faceSize);
    cubemap->mipLevels_ = 1;  // cubemap暂不使用mipmaps

    // 创建Cubemap图像（使用父类的image_和imageMemory_成员）
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = faceSize;
    imageInfo.extent.height = faceSize;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = cubemap->mipLevels_;
    imageInfo.arrayLayers = 6;  // 6个面
    imageInfo.format = cubemap->format_;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;  // 标记为cubemap

    if (vkCreateImage(vulkanContext->device, &imageInfo, nullptr, &cubemap->image_) != VK_SUCCESS) {
        aout << "Error: Failed to create cubemap image!" << std::endl;
        return nullptr;
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(vulkanContext->device, cubemap->image_, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = vulkanContext->findMemoryType(
        memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(vulkanContext->device, &allocInfo, nullptr, &cubemap->imageMemory_) != VK_SUCCESS) {
        aout << "Error: Failed to allocate cubemap image memory!" << std::endl;
        return nullptr;
    }

    vkBindImageMemory(vulkanContext->device, cubemap->image_, cubemap->imageMemory_, 0);

    // 创建staging buffer并复制每个面的数据
    VkDeviceSize imageSize = faceSize * faceSize * 4;  // RGBA
    VkDeviceSize totalBufferSize = imageSize * 6;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    vulkanContext->createBuffer(
        totalBufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory);

    // 将所有面数据复制到staging buffer
    void* data;
    vkMapMemory(vulkanContext->device, stagingBufferMemory, 0, totalBufferSize, 0, &data);
    for (int i = 0; i < 6; i++) {
        memcpy((unsigned char*)data + i * imageSize, faceData[i].pixels.data(), imageSize);
    }
    vkUnmapMemory(vulkanContext->device, stagingBufferMemory);

    // 转换图像布局并复制数据
    VkCommandBuffer cmdBuffer = vulkanContext->beginSingleTimeCommands();

    // 转换到transfer dst optimal
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = cubemap->image_;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 6;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(
        cmdBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    // 复制每个面的数据
    for (uint32_t i = 0; i < 6; i++) {
        VkBufferImageCopy region{};
        region.bufferOffset = i * imageSize;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = i;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {faceSize, faceSize, 1};

        vkCmdCopyBufferToImage(
            cmdBuffer,
            stagingBuffer,
            cubemap->image_,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &region);
    }

    // 转换到shader read only optimal
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
        cmdBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    vulkanContext->endSingleTimeCommands(cmdBuffer);

    vkDestroyBuffer(vulkanContext->device, stagingBuffer, nullptr);
    vkFreeMemory(vulkanContext->device, stagingBufferMemory, nullptr);

    // 创建ImageView (Cubemap view，使用父类的imageView_成员)
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = cubemap->image_;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.format = cubemap->format_;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 6;

    if (vkCreateImageView(vulkanContext->device, &viewInfo, nullptr, &cubemap->imageView_) != VK_SUCCESS) {
        aout << "Error: Failed to create cubemap image view!" << std::endl;
        return nullptr;
    }

    // 创建Sampler (使用父类的sampler_成员)
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(vulkanContext->device, &samplerInfo, nullptr, &cubemap->sampler_) != VK_SUCCESS) {
        aout << "Error: Failed to create cubemap sampler!" << std::endl;
        return nullptr;
    }

    aout << "Cubemap loaded successfully: " << faceSize << "x" << faceSize << std::endl;
    return cubemap;
}
