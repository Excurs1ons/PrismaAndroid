#include <android/imagedecoder.h>
#include "TextureAsset.h"
#include "AndroidOut.h"
#include "Utility.h"
#include "VulkanContext.h" // 需要 Vulkan 上下文管理类
#include <vector>

// 辅助函数：创建 Vulkan 图像
static void createVulkanImage(
        VulkanContext* context,
        uint32_t width,
        uint32_t height,
        uint32_t mipLevels,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkImage& image,
        VkDeviceMemory& imageMemory) {

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(context->device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(context->device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = context->findMemoryType(
            memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(context->device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(context->device, image, imageMemory, 0);
}

// 辅助函数：创建图像视图
static VkImageView createImageView(
        VulkanContext* context,
        VkImage image,
        VkFormat format,
        VkImageAspectFlags aspectFlags,
        uint32_t mipLevels) {

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(context->device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

// 辅助函数：创建采样器
static VkSampler createTextureSampler(VulkanContext* context, uint32_t mipLevels) {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = context->properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(mipLevels);

    VkSampler sampler;
    if (vkCreateSampler(context->device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
    return sampler;
}

// 主要加载函数
std::shared_ptr<TextureAsset> TextureAsset::loadAsset(
        AAssetManager* assetManager,
        const std::string& assetPath,
        VulkanContext* vulkanContext) {

    // 1. 加载图像数据（这部分与 OpenGL 相同）
    auto pAndroidRobotPng = AAssetManager_open(
            assetManager, assetPath.c_str(), AASSET_MODE_BUFFER);

    AImageDecoder* pAndroidDecoder = nullptr;
    auto result = AImageDecoder_createFromAAsset(pAndroidRobotPng, &pAndroidDecoder);
    assert(result == ANDROID_IMAGE_DECODER_SUCCESS);

    AImageDecoder_setAndroidBitmapFormat(pAndroidDecoder, ANDROID_BITMAP_FORMAT_RGBA_8888);

    const AImageDecoderHeaderInfo* pAndroidHeader =
            AImageDecoder_getHeaderInfo(pAndroidDecoder);

    auto width = AImageDecoderHeaderInfo_getWidth(pAndroidHeader);
    auto height = AImageDecoderHeaderInfo_getHeight(pAndroidHeader);
    auto stride = AImageDecoder_getMinimumStride(pAndroidDecoder);

    auto upAndroidImageData = std::make_unique<std::vector<uint8_t>>(height * stride);
    auto decodeResult = AImageDecoder_decodeImage(
            pAndroidDecoder,
            upAndroidImageData->data(),
            stride,
            upAndroidImageData->size());
    assert(decodeResult == ANDROID_IMAGE_DECODER_SUCCESS);

    // 清理解码器
    AImageDecoder_delete(pAndroidDecoder);
    AAsset_close(pAndroidRobotPng);

    // 2. 创建 Vulkan 纹理
    auto texture = std::shared_ptr<TextureAsset>(new TextureAsset(vulkanContext));
    texture->size_ = glm::uvec2(width, height);
    texture->mipLevels_ = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

    if (vulkanContext) {
        // 创建暂存缓冲区并复制数据
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        VkDeviceSize imageSize = width * height * 4; // RGBA

        vulkanContext->createBuffer(
                imageSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer,
                stagingBufferMemory);

        void* data;
        vkMapMemory(vulkanContext->device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, upAndroidImageData->data(), static_cast<size_t>(imageSize));
        vkUnmapMemory(vulkanContext->device, stagingBufferMemory);

        // 3. 创建 Vulkan 图像
        createVulkanImage(
                vulkanContext,
                width,
                height,
                texture->mipLevels_,
                texture->format_,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                texture->image_,
                texture->imageMemory_);

        // 4. 转换图像布局并复制数据
        vulkanContext->transitionImageLayout(
                texture->image_,
                texture->format_,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                texture->mipLevels_);

        vulkanContext->copyBufferToImage(
                stagingBuffer,
                texture->image_,
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height));

        // 5. 生成 Mipmaps
        vulkanContext->generateMipmaps(
                texture->image_,
                texture->format_,
                width,
                height,
                texture->mipLevels_);

        // 6. 清理暂存缓冲区
        vkDestroyBuffer(vulkanContext->device, stagingBuffer, nullptr);
        vkFreeMemory(vulkanContext->device, stagingBufferMemory, nullptr);

        // 7. 创建图像视图和采样器
        texture->imageView_ = createImageView(
                vulkanContext,
                texture->image_,
                texture->format_,
                VK_IMAGE_ASPECT_COLOR_BIT,
                texture->mipLevels_);

        texture->sampler_ = createTextureSampler(vulkanContext, texture->mipLevels_);
    } else {
        // OpenGL 纹理创建
        glGenTextures(1, &texture->textureID_);
        glBindTexture(GL_TEXTURE_2D, texture->textureID_);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     upAndroidImageData->data());
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    return texture;
}

TextureAsset::TextureAsset(VulkanContext* context) : context_(context) {}

TextureAsset::~TextureAsset() {
    Unload();
}

void TextureAsset::Unload() {
    if (context_) {
        vkDestroySampler(context_->device, sampler_, nullptr);
        vkDestroyImageView(context_->device, imageView_, nullptr);
        vkDestroyImage(context_->device, image_, nullptr);
        vkFreeMemory(context_->device, imageMemory_, nullptr);
    }
    if (textureID_ != 0) {
        glDeleteTextures(1, &textureID_);
        textureID_ = 0;
    }
}
