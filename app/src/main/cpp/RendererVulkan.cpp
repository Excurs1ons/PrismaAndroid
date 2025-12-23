#include "RendererVulkan.h"
#include "AndroidOut.h"
#include "Scene.h"
#include "MeshRenderer.h"
#include "ShaderVulkan.h"
#include "TextureAsset.h"
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <vulkan/vulkan_android.h>
#include <vector>
#include <array>
#include <set>
#include <algorithm>
#include <cstring>
#include <chrono>

struct UniformBufferObject {
    alignas(16) Matrix4 model;
    alignas(16) Matrix4 view;
    alignas(16) Matrix4 proj;
};

RendererVulkan::RendererVulkan(android_app *pApp) : app_(pApp) {
    init();
}

RendererVulkan::~RendererVulkan() {
    vkDeviceWaitIdle(vulkanContext_.device);

    scene_.reset();

    // vkDestroyImageView(vulkanContext_.device, textureImageView, nullptr);
    // vkDestroyImage(vulkanContext_.device, textureImage, nullptr);
    // vkFreeMemory(vulkanContext_.device, textureImageMemory, nullptr);
    // vkDestroySampler(vulkanContext_.device, textureSampler, nullptr);

    vkDestroyDescriptorPool(vulkanContext_.device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(vulkanContext_.device, descriptorSetLayout, nullptr);

    for (auto& obj : renderObjects) {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(vulkanContext_.device, obj.uniformBuffers[i], nullptr);
            vkFreeMemory(vulkanContext_.device, obj.uniformBuffersMemory[i], nullptr);
        }

        vkDestroyBuffer(vulkanContext_.device, obj.indexBuffer, nullptr);
        vkFreeMemory(vulkanContext_.device, obj.indexBufferMemory, nullptr);

        vkDestroyBuffer(vulkanContext_.device, obj.vertexBuffer, nullptr);
        vkFreeMemory(vulkanContext_.device, obj.vertexBufferMemory, nullptr);
    }
    renderObjects.clear();

    vkDestroyPipeline(vulkanContext_.device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(vulkanContext_.device, pipelineLayout, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(vulkanContext_.device, vulkanContext_.renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(vulkanContext_.device, vulkanContext_.imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(vulkanContext_.device, vulkanContext_.inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(vulkanContext_.device, vulkanContext_.commandPool, nullptr);

    for (auto framebuffer : vulkanContext_.swapChainFramebuffers) {
        vkDestroyFramebuffer(vulkanContext_.device, framebuffer, nullptr);
    }

    vkDestroyRenderPass(vulkanContext_.device, vulkanContext_.renderPass, nullptr);

    for (auto imageView : vulkanContext_.swapChainImageViews) {
        vkDestroyImageView(vulkanContext_.device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(vulkanContext_.device, vulkanContext_.swapChain, nullptr);
    vkDestroyDevice(vulkanContext_.device, nullptr);
    vkDestroySurfaceKHR(vulkanContext_.instance, vulkanContext_.surface, nullptr);
    vkDestroyInstance(vulkanContext_.instance, nullptr);
}

void RendererVulkan::init() {
    aout << "Initializing Vulkan Renderer" << std::endl;
    
    // 1. Create Instance
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Android";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    std::vector<const char*> instanceExtensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
    };

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();
    createInfo.enabledLayerCount = 0;

    vkCreateInstance(&createInfo, nullptr, &vulkanContext_.instance);

    // 2. Create Surface
    VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo{};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.window = app_->window;
    vkCreateAndroidSurfaceKHR(vulkanContext_.instance, &surfaceCreateInfo, nullptr, &vulkanContext_.surface);

    // 3. Pick Physical Device
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vulkanContext_.instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vulkanContext_.instance, &deviceCount, devices.data());
    vulkanContext_.physicalDevice = devices[0];

    // 4. Create Logical Device
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vulkanContext_.physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(vulkanContext_.physicalDevice, &queueFamilyCount, queueFamilies.data());

    int graphicsFamily = -1;
    int presentFamily = -1;
    for (int i = 0; i < queueFamilies.size(); i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) graphicsFamily = i;
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(vulkanContext_.physicalDevice, i, vulkanContext_.surface, &presentSupport);
        if (presentSupport) presentFamily = i;
        if (graphicsFamily != -1 && presentFamily != -1) break;
    }

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {static_cast<uint32_t>(graphicsFamily), static_cast<uint32_t>(presentFamily)};
    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_FALSE;

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    vkCreateDevice(vulkanContext_.physicalDevice, &deviceCreateInfo, nullptr, &vulkanContext_.device);
    vkGetDeviceQueue(vulkanContext_.device, graphicsFamily, 0, &vulkanContext_.graphicsQueue);
    vkGetDeviceQueue(vulkanContext_.device, presentFamily, 0, &vulkanContext_.presentQueue);

    // 5. Create Swap Chain
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkanContext_.physicalDevice, vulkanContext_.surface, &capabilities);
    VkSurfaceFormatKHR surfaceFormat = {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    vulkanContext_.swapChainImageFormat = surfaceFormat.format;
    vulkanContext_.swapChainExtent = capabilities.currentExtent;
    vulkanContext_.currentTransform = capabilities.currentTransform;

    VkSwapchainCreateInfoKHR swapChainCreateInfo{};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = vulkanContext_.surface;
    swapChainCreateInfo.minImageCount = capabilities.minImageCount + 1;
    swapChainCreateInfo.imageFormat = surfaceFormat.format;
    swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapChainCreateInfo.imageExtent = vulkanContext_.swapChainExtent;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    uint32_t queueFamilyIndices[] = {static_cast<uint32_t>(graphicsFamily), static_cast<uint32_t>(presentFamily)};
    if (graphicsFamily != presentFamily) {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    swapChainCreateInfo.preTransform = capabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    swapChainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapChainCreateInfo.clipped = VK_TRUE;
    
    vkCreateSwapchainKHR(vulkanContext_.device, &swapChainCreateInfo, nullptr, &vulkanContext_.swapChain);

    uint32_t imageCount;
    vkGetSwapchainImagesKHR(vulkanContext_.device, vulkanContext_.swapChain, &imageCount, nullptr);
    vulkanContext_.swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(vulkanContext_.device, vulkanContext_.swapChain, &imageCount, vulkanContext_.swapChainImages.data());

    // 6. Create Image Views
    vulkanContext_.swapChainImageViews.resize(vulkanContext_.swapChainImages.size());
    for (size_t i = 0; i < vulkanContext_.swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = vulkanContext_.swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = vulkanContext_.swapChainImageFormat;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(vulkanContext_.device, &createInfo, nullptr, &vulkanContext_.swapChainImageViews[i]);
    }

    // 7. Create Render Pass
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = vulkanContext_.swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    vkCreateRenderPass(vulkanContext_.device, &renderPassInfo, nullptr, &vulkanContext_.renderPass);

    // 8. Create Framebuffers
    vulkanContext_.swapChainFramebuffers.resize(vulkanContext_.swapChainImageViews.size());
    for (size_t i = 0; i < vulkanContext_.swapChainImageViews.size(); i++) {
        VkImageView attachments[] = { vulkanContext_.swapChainImageViews[i] };
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = vulkanContext_.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = vulkanContext_.swapChainExtent.width;
        framebufferInfo.height = vulkanContext_.swapChainExtent.height;
        framebufferInfo.layers = 1;
        vkCreateFramebuffer(vulkanContext_.device, &framebufferInfo, nullptr, &vulkanContext_.swapChainFramebuffers[i]);
    }

    // 9. Create Command Pool
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = graphicsFamily;
    vkCreateCommandPool(vulkanContext_.device, &poolInfo, nullptr, &vulkanContext_.commandPool);

    // 10. Create Sync Objects
    vulkanContext_.imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    vulkanContext_.renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    vulkanContext_.inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkCreateSemaphore(vulkanContext_.device, &semaphoreInfo, nullptr, &vulkanContext_.imageAvailableSemaphores[i]);
        vkCreateSemaphore(vulkanContext_.device, &semaphoreInfo, nullptr, &vulkanContext_.renderFinishedSemaphores[i]);
        vkCreateFence(vulkanContext_.device, &fenceInfo, nullptr, &vulkanContext_.inFlightFences[i]);
    }

    createScene();
    createGraphicsPipeline();
    // createTextureImage(); // Handled by TextureAsset
    // createTextureImageView(); // Handled by TextureAsset
    // createTextureSampler(); // Handled by TextureAsset
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();

    aout << "Vulkan Initialized Successfully" << std::endl;
}

void RendererVulkan::createScene() {
    scene_ = std::make_unique<Scene>();
    auto assetManager = app_->activity->assetManager;

    // 1. Robot
    {
//        auto go = std::make_shared<GameObject>();
//        go->name = "Robot";
//        go->position = Vector3(0, 0, 0);
//
//        auto texture = TextureAsset::loadAsset(assetManager, "android_robot.png", &vulkanContext_);
//
//        std::vector<Vertex> vertices = {
//                Vertex(Vector3(0.5f, 0.5f, 0.0f), Vector2(1.0f, 0.0f)),
//                Vertex(Vector3(-0.5f, 0.5f, 0.0f), Vector2(0.0f, 0.0f)),
//                Vertex(Vector3(-0.5f, -0.5f, 0.0f), Vector2(0.0f, 1.0f)),
//                Vertex(Vector3(0.5f, -0.5f, 0.0f), Vector2(1.0f, 1.0f))
//        };
//        std::vector<Index> indices = { 0, 1, 2, 0, 2, 3 };
//
//        auto model = std::make_shared<Model>(vertices, indices, texture);
//        go->addComponent(std::make_shared<MeshRenderer>(model));
//        scene_->addGameObject(go);
    }

    // 2. Cube
    {
        auto go = std::make_shared<GameObject>();
        go->name = "Cube";
        go->position = Vector3(0, 0, -2.0f); // Behind the robot
        
        auto texture = TextureAsset::loadAsset(assetManager, "android_robot.png", &vulkanContext_);

        Vector3 red(1.0f, 1.0f, 1.0f);
        std::vector<Vertex> vertices = {
            // Front
            Vertex(Vector3(-0.5f, -0.5f,  0.5f), red, Vector2(0.0f, 0.0f)),
            Vertex(Vector3( 0.5f, -0.5f,  0.5f), red, Vector2(1.0f, 0.0f)),
            Vertex(Vector3( 0.5f,  0.5f,  0.5f), red, Vector2(1.0f, 1.0f)),
            Vertex(Vector3(-0.5f,  0.5f,  0.5f), red, Vector2(0.0f, 1.0f)),
            // Back
            Vertex(Vector3( 0.5f, -0.5f, -0.5f), red, Vector2(0.0f, 0.0f)),
            Vertex(Vector3(-0.5f, -0.5f, -0.5f), red, Vector2(1.0f, 0.0f)),
            Vertex(Vector3(-0.5f,  0.5f, -0.5f), red, Vector2(1.0f, 1.0f)),
            Vertex(Vector3( 0.5f,  0.5f, -0.5f), red, Vector2(0.0f, 1.0f)),
            // Top
            Vertex(Vector3(-0.5f,  0.5f, -0.5f), red, Vector2(0.0f, 0.0f)),
            Vertex(Vector3(-0.5f,  0.5f,  0.5f), red, Vector2(0.0f, 1.0f)),
            Vertex(Vector3( 0.5f,  0.5f,  0.5f), red, Vector2(1.0f, 1.0f)),
            Vertex(Vector3( 0.5f,  0.5f, -0.5f), red, Vector2(1.0f, 0.0f)),
            // Bottom
            Vertex(Vector3(-0.5f, -0.5f, -0.5f), red, Vector2(0.0f, 0.0f)),
            Vertex(Vector3( 0.5f, -0.5f, -0.5f), red, Vector2(1.0f, 0.0f)),
            Vertex(Vector3( 0.5f, -0.5f,  0.5f), red, Vector2(1.0f, 1.0f)),
            Vertex(Vector3(-0.5f, -0.5f,  0.5f), red, Vector2(0.0f, 1.0f)),
            // Right
            Vertex(Vector3( 0.5f, -0.5f, -0.5f), red, Vector2(0.0f, 0.0f)),
            Vertex(Vector3( 0.5f,  0.5f, -0.5f), red, Vector2(1.0f, 0.0f)),
            Vertex(Vector3( 0.5f,  0.5f,  0.5f), red, Vector2(1.0f, 1.0f)),
            Vertex(Vector3( 0.5f, -0.5f,  0.5f), red, Vector2(0.0f, 1.0f)),
            // Left
            Vertex(Vector3(-0.5f, -0.5f, -0.5f), red, Vector2(0.0f, 0.0f)),
            Vertex(Vector3(-0.5f, -0.5f,  0.5f), red, Vector2(1.0f, 0.0f)),
            Vertex(Vector3(-0.5f,  0.5f,  0.5f), red, Vector2(1.0f, 1.0f)),
            Vertex(Vector3(-0.5f,  0.5f, -0.5f), red, Vector2(0.0f, 1.0f)),
        };

        std::vector<Index> indices = {
            0, 1, 2, 2, 3, 0,       // Front
            4, 5, 6, 6, 7, 4,       // Back
            8, 9, 10, 10, 11, 8,    // Top
            12, 13, 14, 14, 15, 12, // Bottom
            16, 17, 18, 18, 19, 16, // Right
            20, 21, 22, 22, 23, 20  // Left
        };

        auto model = std::make_shared<Model>(vertices, indices, texture);
        go->addComponent(std::make_shared<MeshRenderer>(model));
        scene_->addGameObject(go);
    }
}

void RendererVulkan::createGraphicsPipeline() {
    auto vertShaderCode = ShaderVulkan::loadShader(app_->activity->assetManager, "shaders/shader.vert.spv");
    auto fragShaderCode = ShaderVulkan::loadShader(app_->activity->assetManager, "shaders/shader.frag.spv");

    if (vertShaderCode.empty() || fragShaderCode.empty()) {
        aout << "Failed to load shader files!" << std::endl;
        return;
    }

    VkShaderModule vertShaderModule;
    VkShaderModuleCreateInfo vertCreateInfo{};
    vertCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertCreateInfo.codeSize = vertShaderCode.size() * 4;
    vertCreateInfo.pCode = vertShaderCode.data();
    vkCreateShaderModule(vulkanContext_.device, &vertCreateInfo, nullptr, &vertShaderModule);

    VkShaderModule fragShaderModule;
    VkShaderModuleCreateInfo fragCreateInfo{};
    fragCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragCreateInfo.codeSize = fragShaderCode.size() * 4;
    fragCreateInfo.pCode = fragShaderCode.data();
    vkCreateShaderModule(vulkanContext_.device, &fragCreateInfo, nullptr, &fragShaderModule);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, uv);

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) vulkanContext_.swapChainExtent.width;
    viewport.height = (float) vulkanContext_.swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = vulkanContext_.swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    // Enable double-sided rendering by disabling culling
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    vkCreateDescriptorSetLayout(vulkanContext_.device, &layoutInfo, nullptr, &descriptorSetLayout);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

    vkCreatePipelineLayout(vulkanContext_.device, &pipelineLayoutInfo, nullptr, &pipelineLayout);

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = vulkanContext_.renderPass;
    pipelineInfo.subpass = 0;

    vkCreateGraphicsPipelines(vulkanContext_.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline);

    vkDestroyShaderModule(vulkanContext_.device, fragShaderModule, nullptr);
    vkDestroyShaderModule(vulkanContext_.device, vertShaderModule, nullptr);
}

void RendererVulkan::createTextureImage() {
    // Handled by TextureAsset
}

void RendererVulkan::createTextureImageView() {
    // Handled by TextureAsset
}

void RendererVulkan::createTextureSampler() {
    // Handled by TextureAsset
}

void RendererVulkan::createVertexBuffer() {
    auto& gameObjects = scene_->getGameObjects();
    renderObjects.resize(gameObjects.size());

    for (size_t i = 0; i < gameObjects.size(); i++) {
        auto go = gameObjects[i];
        auto meshRenderer = go->getComponent<MeshRenderer>();
        auto model = meshRenderer->getModel();
        VkDeviceSize bufferSize = sizeof(Vertex) * model->getVertexCount();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        vulkanContext_.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(vulkanContext_.device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, model->getVertexData(), (size_t) bufferSize);
        vkUnmapMemory(vulkanContext_.device, stagingBufferMemory);

        vulkanContext_.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, renderObjects[i].vertexBuffer, renderObjects[i].vertexBufferMemory);

        vulkanContext_.copyBuffer(stagingBuffer, renderObjects[i].vertexBuffer, bufferSize);

        vkDestroyBuffer(vulkanContext_.device, stagingBuffer, nullptr);
        vkFreeMemory(vulkanContext_.device, stagingBufferMemory, nullptr);
    }
}

void RendererVulkan::createIndexBuffer() {
    auto& gameObjects = scene_->getGameObjects();
    
    for (size_t i = 0; i < gameObjects.size(); i++) {
        auto go = gameObjects[i];
        auto meshRenderer = go->getComponent<MeshRenderer>();
        auto model = meshRenderer->getModel();
        VkDeviceSize bufferSize = sizeof(Index) * model->getIndexCount();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        vulkanContext_.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(vulkanContext_.device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, model->getIndexData(), (size_t) bufferSize);
        vkUnmapMemory(vulkanContext_.device, stagingBufferMemory);

        vulkanContext_.createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, renderObjects[i].indexBuffer, renderObjects[i].indexBufferMemory);

        vulkanContext_.copyBuffer(stagingBuffer, renderObjects[i].indexBuffer, bufferSize);

        vkDestroyBuffer(vulkanContext_.device, stagingBuffer, nullptr);
        vkFreeMemory(vulkanContext_.device, stagingBufferMemory, nullptr);
    }
}

void RendererVulkan::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    auto& gameObjects = scene_->getGameObjects();

    for (size_t i = 0; i < gameObjects.size(); i++) {
        renderObjects[i].uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        renderObjects[i].uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        renderObjects[i].uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t j = 0; j < MAX_FRAMES_IN_FLIGHT; j++) {
            vulkanContext_.createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, renderObjects[i].uniformBuffers[j], renderObjects[i].uniformBuffersMemory[j]);
            vkMapMemory(vulkanContext_.device, renderObjects[i].uniformBuffersMemory[j], 0, bufferSize, 0, &renderObjects[i].uniformBuffersMapped[j]);
        }
    }
}

void RendererVulkan::createDescriptorPool() {
    auto& gameObjects = scene_->getGameObjects();
    uint32_t count = static_cast<uint32_t>(gameObjects.size() * MAX_FRAMES_IN_FLIGHT);

    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = count;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = count;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = count;

    vkCreateDescriptorPool(vulkanContext_.device, &poolInfo, nullptr, &descriptorPool);
}

void RendererVulkan::createDescriptorSets() {
    auto& gameObjects = scene_->getGameObjects();
    
    for (size_t i = 0; i < gameObjects.size(); i++) {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        renderObjects[i].descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        vkAllocateDescriptorSets(vulkanContext_.device, &allocInfo, renderObjects[i].descriptorSets.data());

        for (size_t j = 0; j < MAX_FRAMES_IN_FLIGHT; j++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = renderObjects[i].uniformBuffers[j];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            auto go = gameObjects[i];
            auto meshRenderer = go->getComponent<MeshRenderer>();
            auto& textureAsset = meshRenderer->getModel()->getTexture();

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = textureAsset.getImageView();
            imageInfo.sampler = textureAsset.getSampler();

            std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = renderObjects[i].descriptorSets[j];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = renderObjects[i].descriptorSets[j];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(vulkanContext_.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }
}

void RendererVulkan::createCommandBuffers() {
    vulkanContext_.commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = vulkanContext_.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) vulkanContext_.commandBuffers.size();

    vkAllocateCommandBuffers(vulkanContext_.device, &allocInfo, vulkanContext_.commandBuffers.data());
}

/**
 * @brief 更新 Uniform Buffer 数据（每帧调用）
 *
 * Uniform Buffer 包含三个矩阵：
 * - model: 模型矩阵（物体本地坐标 → 世界坐标）
 * - view: 视图矩阵（世界坐标 → 相机坐标）
 * - proj: 投影矩阵（相机坐标 → 裁剪空间）
 *
 * @param currentImage 当前帧索引（用于 Flight Frame 机制）
 *
 * 重要：投影矩阵每帧都会根据 swapChainExtent 重新计算宽高比
 * 这确保了当屏幕旋转时，投影矩阵会自动适应新的屏幕方向
 */
void RendererVulkan::updateUniformBuffer(uint32_t currentImage) {
    // 获取时间（用于动画）
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    auto& gameObjects = scene_->getGameObjects();
    for (size_t i = 0; i < gameObjects.size(); i++) {
        auto go = gameObjects[i];

        // 更新立方体旋转动画
        if (go->name == "Cube") {
            go->rotation.x = time * 30.0f;
            go->rotation.y = time * 30.0f;
        }

        // ============ 构建 Uniform Buffer Object ============
        UniformBufferObject ubo{};

        // 1. Model 矩阵：物体变换矩阵
        ubo.model = go->getTransformMatrix();

        // 2. View 矩阵：相机位置和方向
        //    glm::lookAt(相机位置, 目标位置, 世界空间上方向)
        //    相机位于 (0, 0, 3)，看向原点 (0, 0, 0)，Y 轴向上
        ubo.view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 3.0f),  // 相机位置
            glm::vec3(0.0f, 0.0f, 0.0f),  // 目标位置
            glm::vec3(0.0f, 1.0f, 0.0f)); // 上方向

        // 3. Projection 矩阵：透视投影
        //    glm::perspective(FOV, 宽高比, 近平面距离, 远平面距离)
        //
        //    重要：在横屏模式下，当 currentTransform 包含 ROTATE_90 或 ROTATE_270 时，
        //    swapChainExtent 的宽高是交换的（例如 1080x1920 而不是 1920x1080），
        //    因为 SwapChain 图像是垂直存储的，然后通过 preTransform 旋转显示。
        //    因此我们需要根据 transform 决定是否交换宽高比。
        float aspectRatio;
        if (vulkanContext_.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR ||
            vulkanContext_.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR) {
            // 90度或270度旋转：SwapChain 图像的宽高与显示方向相反，需要交换
            aspectRatio = vulkanContext_.swapChainExtent.height / (float) vulkanContext_.swapChainExtent.width;
        } else {
            // 无旋转或180度旋转：直接使用 SwapChain 的宽高比
            aspectRatio = vulkanContext_.swapChainExtent.width / (float) vulkanContext_.swapChainExtent.height;
        }

        // 调试输出（仅第一帧）
        static bool debugPrinted = false;
        if (!debugPrinted && i == 0) {
            aout << "Projection Debug: swapChainExtent=" << vulkanContext_.swapChainExtent.width
                 << "x" << vulkanContext_.swapChainExtent.height
                 << ", transform=" << vulkanContext_.currentTransform
                 << ", aspectRatio=" << aspectRatio << std::endl;
            debugPrinted = true;
        }

        ubo.proj = glm::perspective(
            glm::radians(45.0f),   // 45 度视野角
            aspectRatio,            // 动态宽高比（横屏时 >1，竖屏时 <1）
            0.1f,                   // 近平面距离
            10.0f);                 // 远平面距离

        // Vulkan 的 Y 轴方向与 OpenGL 相反，需要翻转 Y 轴
        // OpenGL: Y 轴向上，Vulkan: Y 轴向下
        ubo.proj[1][1] *= -1;

        // 将 UBO 数据复制到 Uniform Buffer（已映射的内存）
        memcpy(renderObjects[i].uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }
}
void RendererVulkan::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = vulkanContext_.renderPass;
    renderPassInfo.framebuffer = vulkanContext_.swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = vulkanContext_.swapChainExtent;

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    // 【新增】动态设置 Viewport 和 Scissor
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)vulkanContext_.swapChainExtent.width;
    viewport.height = (float)vulkanContext_.swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = vulkanContext_.swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    auto& gameObjects = scene_->getGameObjects();
    for (size_t i = 0; i < gameObjects.size(); i++) {
        auto go = gameObjects[i];
        auto meshRenderer = go->getComponent<MeshRenderer>();
        auto model = meshRenderer->getModel();

        VkBuffer vertexBuffers[] = {renderObjects[i].vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

        vkCmdBindIndexBuffer(commandBuffer, renderObjects[i].indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &renderObjects[i].descriptorSets[currentFrame], 0, nullptr);
        
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(model->getIndexCount()), 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(commandBuffer);

    vkEndCommandBuffer(commandBuffer);
}

/**
 * @brief 主渲染函数
 *
 * 每帧调用一次，负责：
 * 1. 获取下一个 SwapChain 图像
 * 2. 更新 Uniform Buffer（包含投影矩阵）
 * 3. 录制命令缓冲区
 * 4. 提交渲染命令
 * 5. 呈现图像到屏幕
 *
 * 当屏幕旋转或窗口尺寸改变时，会自动重建 SwapChain 及相关资源
 */
void RendererVulkan::render() {
    // 等待上一帧完成渲染（防止帧之间的资源竞争）
    vkWaitForFences(vulkanContext_.device, 1, &vulkanContext_.inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    // ============ 1. 获取 SwapChain 中的下一个可用图像 ============
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        vulkanContext_.device,
        vulkanContext_.swapChain,
        UINT64_MAX,                                              // 超时时间（无限等待）
        vulkanContext_.imageAvailableSemaphores[currentFrame],   // 图像可用时发出信号
        VK_NULL_HANDLE,
        &imageIndex);

    // ============ 2. 处理屏幕旋转（SwapChain 重建）============
    // VK_ERROR_OUT_OF_DATE_KHR: SwapChain 不再与 Surface 兼容（如屏幕旋转）
    // VK_SUBOPTIMAL_KHR: SwapChain 仍然可用但不是最优状态
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // 屏幕旋转或尺寸改变，需要重建 SwapChain
        aout << "Detected screen rotation/resize, recreating SwapChain..." << std::endl;
        recreateSwapChain();
        return;  // 跳过当前帧，下一帧使用新的 SwapChain 渲染
    } else if (result != VK_SUCCESS) {
        // 其他错误，直接返回
        aout << "Failed to acquire swap chain image: " << result << std::endl;
        return;
    }

    // ============ 3. 更新 Uniform Buffer（包含投影矩阵）============
    //    投影矩阵会根据 swapChainExtent.width/height 自动计算正确的宽高比
    updateUniformBuffer(currentFrame);

    // ============ 4. 重置 Fence 和命令缓冲区 ============
    vkResetFences(vulkanContext_.device, 1, &vulkanContext_.inFlightFences[currentFrame]);
    vkResetCommandBuffer(vulkanContext_.commandBuffers[currentFrame], 0);

    // ============ 5. 录制渲染命令 ============
    recordCommandBuffer(vulkanContext_.commandBuffers[currentFrame], imageIndex);

    // ============ 6. 提交渲染命令到图形队列 ============
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    // 等待信号量配置（等待图像可用后再开始渲染）
    VkSemaphore waitSemaphores[] = {vulkanContext_.imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    // 命令缓冲区配置
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vulkanContext_.commandBuffers[currentFrame];

    // 信号量配置（渲染完成后发出信号）
    VkSemaphore signalSemaphores[] = {vulkanContext_.renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // 提交命令
    vkQueueSubmit(vulkanContext_.graphicsQueue, 1, &submitInfo, vulkanContext_.inFlightFences[currentFrame]);

    // ============ 7. 呈现图像到屏幕 ============
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    // 等待渲染完成后再呈现
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    // SwapChain 配置
    VkSwapchainKHR swapChains[] = {vulkanContext_.swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    // 呈现图像
    result = vkQueuePresentKHR(vulkanContext_.presentQueue, &presentInfo);

    // 检查呈现结果，如果屏幕旋转则重建 SwapChain
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        aout << "Detected screen rotation/resize after present, recreating SwapChain..." << std::endl;
        recreateSwapChain();
    }

    // ============ 8. 更新帧索引 ============
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

/**
 * @brief 清理旧的 SwapChain 相关资源
 *
 * 当屏幕旋转或窗口尺寸改变时，需要先清理旧的 SwapChain、ImageViews、Framebuffers
 * 然后重新创建它们以适应新的屏幕尺寸。
 *
 * 注意：这里不清理 CommandPool 和 Sync Objects，因为它们不依赖于 SwapChain 的尺寸
 */
void RendererVulkan::cleanupSwapChain() {
    // 等待设备空闲，确保所有渲染操作完成
    vkDeviceWaitIdle(vulkanContext_.device);

    // 1. 清理 Framebuffers（依赖于 SwapChain ImageViews）
    for (auto framebuffer : vulkanContext_.swapChainFramebuffers) {
        vkDestroyFramebuffer(vulkanContext_.device, framebuffer, nullptr);
    }
    vulkanContext_.swapChainFramebuffers.clear();

    // 2. 清理 Graphics Pipeline
    //    必须清理，因为 Pipeline 中包含了 Viewport，而 Viewport 依赖于 SwapChain 尺寸
    vkDestroyPipeline(vulkanContext_.device, graphicsPipeline, nullptr);
    graphicsPipeline = VK_NULL_HANDLE;

    // 3. 清理 Pipeline Layout
    vkDestroyPipelineLayout(vulkanContext_.device, pipelineLayout, nullptr);
    pipelineLayout = VK_NULL_HANDLE;

    // 4. 清理 Render Pass
    //    虽然 Render Pass 本身不直接依赖 SwapChain 尺寸，但为了完整性我们重新创建
    vkDestroyRenderPass(vulkanContext_.device, vulkanContext_.renderPass, nullptr);
    vulkanContext_.renderPass = VK_NULL_HANDLE;

    // 5. 清理 SwapChain ImageViews（依赖于 SwapChain Images）
    for (auto imageView : vulkanContext_.swapChainImageViews) {
        vkDestroyImageView(vulkanContext_.device, imageView, nullptr);
    }
    vulkanContext_.swapChainImageViews.clear();

    // 6. 最后清理 SwapChain 本身
    vkDestroySwapchainKHR(vulkanContext_.device, vulkanContext_.swapChain, nullptr);
    vulkanContext_.swapChain = VK_NULL_HANDLE;
}

/**
 * @brief 重建 SwapChain 及其相关资源
 *
 * 当屏幕旋转或窗口尺寸改变时调用。此函数会：
 * 1. 重新获取 Surface Capabilities 以获取新的屏幕尺寸
 * 2. 重新创建 SwapChain
 * 3. 重新创建 ImageViews
 * 4. 重新创建 Render Pass
 * 5. 重新创建 Graphics Pipeline（包含更新后的 Viewport）
 * 6. 重新创建 Framebuffers
 *
 * 投影矩阵会在 updateUniformBuffer() 中自动更新，因为它每帧都会重新计算宽高比
 */
void RendererVulkan::recreateSwapChain() {
    // 获取当前 Surface 的能力信息，包含最新的屏幕尺寸
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        vulkanContext_.physicalDevice,
        vulkanContext_.surface,
        &capabilities);


    int32_t windowWidth = ANativeWindow_getWidth(app_->window);
    int32_t windowHeight = ANativeWindow_getHeight(app_->window);

    // 如果 Vulkan 返回的分辨率和 Window 的分辨率不一致，说明 Surface 还没准备好
    // 我们强制使用 Window 的分辨率（前提是 capabilities 允许）
    if (capabilities.currentExtent.width != windowWidth || capabilities.currentExtent.height != windowHeight) {
        // 更新 capabilities，或者稍作等待
        // 在这里我们手动修正 extent 为窗口的实际大小
        capabilities.currentExtent.width = windowWidth;
        capabilities.currentExtent.height = windowHeight;
        aout << "Applying Width - Height Correction: " << windowWidth << "x" << windowHeight << std::endl;
    }

    // 更新 SwapChain 尺寸为当前最新的屏幕尺寸
    vulkanContext_.swapChainExtent = capabilities.currentExtent;
    vulkanContext_.currentTransform = capabilities.currentTransform;

    // 处理最小化的情况
    if (capabilities.currentExtent.width == 0 || capabilities.currentExtent.height == 0) {
        return;
    }
    // ===================================

    // 保存旧的 SwapChain 句柄，用于创建新链时的 oldSwapchain 参数，以及后续销毁
    VkSwapchainKHR oldSwapChain = vulkanContext_.swapChain;

    // 获取 Surface 格式
    VkSurfaceFormatKHR surfaceFormat = {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    vulkanContext_.swapChainImageFormat = surfaceFormat.format;

    // ============ 1. 创建新的 SwapChain ============
    VkSwapchainCreateInfoKHR swapChainCreateInfo{};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = vulkanContext_.surface;
    swapChainCreateInfo.minImageCount = capabilities.minImageCount + 1;
    swapChainCreateInfo.imageFormat = surfaceFormat.format;
    swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapChainCreateInfo.imageExtent = vulkanContext_.swapChainExtent;  // 使用新尺寸
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // 获取队列族索引（与 init() 中相同的逻辑）
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vulkanContext_.physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(vulkanContext_.physicalDevice, &queueFamilyCount, queueFamilies.data());

    int graphicsFamily = -1;
    int presentFamily = -1;
    for (int i = 0; i < queueFamilies.size(); i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) graphicsFamily = i;
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(vulkanContext_.physicalDevice, i, vulkanContext_.surface, &presentSupport);
        if (presentSupport) presentFamily = i;
        if (graphicsFamily != -1 && presentFamily != -1) break;
    }

    uint32_t queueFamilyIndices[] = {
        static_cast<uint32_t>(graphicsFamily),
        static_cast<uint32_t>(presentFamily)
    };

    if (graphicsFamily != presentFamily) {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    swapChainCreateInfo.preTransform = capabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    swapChainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapChainCreateInfo.clipped = VK_TRUE;
    // 设置旧 SwapChain 以实现平滑过渡（Vulkan 会在获取新图像后才销毁旧 SwapChain）
    swapChainCreateInfo.oldSwapchain = oldSwapChain;

    // 创建新 SwapChain
    VkResult result = vkCreateSwapchainKHR(
        vulkanContext_.device,
        &swapChainCreateInfo,
        nullptr,
        &vulkanContext_.swapChain);

    if (result != VK_SUCCESS) {
        aout << "Failed to recreate swapchain: " << result << std::endl;
        return;
    }
    // 【重要】创建完新链后，必须销毁旧链
    if (oldSwapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(vulkanContext_.device, oldSwapChain, nullptr);
    }
    // 获取新 SwapChain 的图像
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(vulkanContext_.device, vulkanContext_.swapChain, &imageCount, nullptr);
    vulkanContext_.swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(vulkanContext_.device, vulkanContext_.swapChain, &imageCount, vulkanContext_.swapChainImages.data());

    // ============ 2. 创建新的 ImageViews ============
    vulkanContext_.swapChainImageViews.resize(vulkanContext_.swapChainImages.size());
    for (size_t i = 0; i < vulkanContext_.swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = vulkanContext_.swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = vulkanContext_.swapChainImageFormat;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(vulkanContext_.device, &createInfo, nullptr, &vulkanContext_.swapChainImageViews[i]);
    }

    // ============ 3. 创建新的 Render Pass ============
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = vulkanContext_.swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    vkCreateRenderPass(vulkanContext_.device, &renderPassInfo, nullptr, &vulkanContext_.renderPass);

    // ============ 4. 创建新的 Graphics Pipeline（包含更新后的 Viewport）============
    //    重新加载着色器代码
    auto vertShaderCode = ShaderVulkan::loadShader(app_->activity->assetManager, "shaders/shader.vert.spv");
    auto fragShaderCode = ShaderVulkan::loadShader(app_->activity->assetManager, "shaders/shader.frag.spv");

    if (vertShaderCode.empty() || fragShaderCode.empty()) {
        aout << "Failed to load shader files during swapchain recreation!" << std::endl;
        return;
    }

    // 创建着色器模块
    VkShaderModule vertShaderModule;
    VkShaderModuleCreateInfo vertCreateInfo{};
    vertCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertCreateInfo.codeSize = vertShaderCode.size() * 4;
    vertCreateInfo.pCode = vertShaderCode.data();
    vkCreateShaderModule(vulkanContext_.device, &vertCreateInfo, nullptr, &vertShaderModule);

    VkShaderModule fragShaderModule;
    VkShaderModuleCreateInfo fragCreateInfo{};
    fragCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragCreateInfo.codeSize = fragShaderCode.size() * 4;
    fragCreateInfo.pCode = fragShaderCode.data();
    vkCreateShaderModule(vulkanContext_.device, &fragCreateInfo, nullptr, &fragShaderModule);

    // 着色器阶段
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    // 顶点输入状态（与 init() 中相同）
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, uv);

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // 输入装配状态
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // ============ 关键：使用新的屏幕尺寸更新 Viewport ============
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) vulkanContext_.swapChainExtent.width;   // 新的宽度
    viewport.height = (float) vulkanContext_.swapChainExtent.height; // 新的高度
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = vulkanContext_.swapChainExtent;
    /* Viewport 设置 */
//    VkPipelineViewportStateCreateInfo viewportState{};
//    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
//    viewportState.viewportCount = 1;
//    viewportState.pViewports = &viewport;
//    viewportState.scissorCount = 1;
//    viewportState.pScissors = &scissor;

    // 移除原来的 Viewport 设置，改为只指定数量
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // 添加 Dynamic State 定义
    std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // 光栅化状态
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // 多重采样状态
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // 颜色混合状态
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // 创建 Pipeline Layout（与 init() 中相同）
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

    vkCreatePipelineLayout(vulkanContext_.device, &pipelineLayoutInfo, nullptr, &pipelineLayout);

    // 创建 Graphics Pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = vulkanContext_.renderPass;
    pipelineInfo.subpass = 0;

    pipelineInfo.pDynamicState = &dynamicState;

    vkCreateGraphicsPipelines(vulkanContext_.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline);

    // 清理着色器模块
    vkDestroyShaderModule(vulkanContext_.device, fragShaderModule, nullptr);
    vkDestroyShaderModule(vulkanContext_.device, vertShaderModule, nullptr);

    // ============ 5. 创建新的 Framebuffers ============
    vulkanContext_.swapChainFramebuffers.resize(vulkanContext_.swapChainImageViews.size());
    for (size_t i = 0; i < vulkanContext_.swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {vulkanContext_.swapChainImageViews[i]};
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = vulkanContext_.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = vulkanContext_.swapChainExtent.width;   // 新的宽度
        framebufferInfo.height = vulkanContext_.swapChainExtent.height; // 新的高度
        framebufferInfo.layers = 1;
        vkCreateFramebuffer(vulkanContext_.device, &framebufferInfo, nullptr, &vulkanContext_.swapChainFramebuffers[i]);
    }

    aout << "SwapChain recreated successfully with new size: "
         << vulkanContext_.swapChainExtent.width << "x"
         << vulkanContext_.swapChainExtent.height << std::endl;
}