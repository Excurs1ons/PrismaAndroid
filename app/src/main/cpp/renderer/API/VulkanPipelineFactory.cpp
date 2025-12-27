#include "VulkanPipelineFactory.h"
#include "../../ShaderVulkan.h"
#include "../../AndroidOut.h"
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <stdexcept>

#if RENDER_API_VULKAN

std::vector<uint32_t> VulkanPipelineFactory::loadShader(const std::string& path) {
    return ShaderVulkan::loadShader(app_->activity->assetManager, path.c_str());
}

VkShaderModule VulkanPipelineFactory::createShaderModule(VkDevice device, const std::vector<uint32_t>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size() * 4;
    createInfo.pCode = code.data();

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module!");
    }
    return shaderModule;
}

GraphicsPipeline* VulkanPipelineFactory::createGraphicsPipeline(
    const GraphicsPipelineConfig& config,
    NativeDevice device,
    NativeRenderPass renderPass,
    void* shaderData)
{
    VkDevice vkDevice = static_cast<VkDevice>(device);
    VkRenderPass vkRenderPass = static_cast<VkRenderPass>(renderPass);

    // 加载着色器
    auto vertShaderCode = loadShader(config.vertexShaderPath);
    auto fragShaderCode = loadShader(config.fragmentShaderPath);

    if (vertShaderCode.empty() || fragShaderCode.empty()) {
        throw std::runtime_error("Failed to load shader files!");
    }

    // 创建着色器模块
    VkShaderModule vertShaderModule = createShaderModule(vkDevice, vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(vkDevice, fragShaderCode);

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

    // 顶点输入状态
    std::vector<VkVertexInputBindingDescription> bindingDescs;
    std::vector<VkVertexInputAttributeDescription> attrDescs;

    for (const auto& binding : config.vertexBindings) {
        VkVertexInputBindingDescription desc{};
        desc.binding = binding.binding;
        desc.stride = binding.stride;
        desc.inputRate = binding.perInstance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
        bindingDescs.push_back(desc);
    }

    for (const auto& attr : config.vertexAttributes) {
        VkVertexInputAttributeDescription desc{};
        desc.location = attr.location;
        desc.binding = attr.binding;
        desc.format = vulkanVertexFormat(attr.format);
        desc.offset = attr.offset;
        attrDescs.push_back(desc);
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescs.size());
    vertexInputInfo.pVertexBindingDescriptions = bindingDescs.data();
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDescs.size());
    vertexInputInfo.pVertexAttributeDescriptions = attrDescs.data();

    // 输入装配状态
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = vulkanTopology(config.topology);
    inputAssembly.primitiveRestartEnable = config.primitiveRestartEnable;

    // 视口和裁剪（动态状态）
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // 光栅化状态
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = config.depthClampEnable;
    rasterizer.rasterizerDiscardEnable = config.rasterizerDiscardEnable;
    rasterizer.polygonMode = vulkanPolygonMode(config.polygonMode);
    rasterizer.lineWidth = config.lineWidth;
    rasterizer.cullMode = vulkanCullMode(config.cullMode);
    rasterizer.frontFace = vulkanFrontFace(config.frontFace);
    rasterizer.depthBiasEnable = config.depthBiasEnable;

    // 多重采样状态
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = config.sampleShadingEnable;
    multisampling.rasterizationSamples = static_cast<VkSampleCountFlagBits>(config.rasterizationSamples);

    // 深度模板状态
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = config.depthTestEnable;
    depthStencil.depthWriteEnable = config.depthWriteEnable;
    depthStencil.depthBoundsTestEnable = config.depthBoundsTestEnable;
    depthStencil.stencilTestEnable = config.stencilTestEnable;

    // 颜色混合附件
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = config.blendAttachment.colorWriteMask;
    colorBlendAttachment.blendEnable = config.blendAttachment.blendEnable;
    colorBlendAttachment.srcColorBlendFactor = vulkanBlendFactor(config.blendAttachment.srcColorBlendFactor);
    colorBlendAttachment.dstColorBlendFactor = vulkanBlendFactor(config.blendAttachment.dstColorBlendFactor);
    colorBlendAttachment.colorBlendOp = vulkanBlendOp(config.blendAttachment.colorBlendOp);
    colorBlendAttachment.srcAlphaBlendFactor = vulkanBlendFactor(config.blendAttachment.srcAlphaBlendFactor);
    colorBlendAttachment.dstAlphaBlendFactor = vulkanBlendFactor(config.blendAttachment.dstAlphaBlendFactor);
    colorBlendAttachment.alphaBlendOp = vulkanBlendOp(config.blendAttachment.alphaBlendOp);

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = config.logicOpEnable;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Pipeline Layout
    VkDescriptorSetLayout layout = static_cast<VkDescriptorSetLayout>(config.descriptorSetLayout);
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = (layout != VK_NULL_HANDLE) ? 1 : 0;
    pipelineLayoutInfo.pSetLayouts = (layout != VK_NULL_HANDLE) ? &layout : nullptr;

    VkPipelineLayout pipelineLayout;
    if (vkCreatePipelineLayout(vkDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

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
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = vkRenderPass;
    pipelineInfo.subpass = 0;

    VkPipeline graphicsPipeline;
    if (vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        vkDestroyPipelineLayout(vkDevice, pipelineLayout, nullptr);
        throw std::runtime_error("Failed to create graphics pipeline!");
    }

    // 清理着色器模块
    vkDestroyShaderModule(vkDevice, fragShaderModule, nullptr);
    vkDestroyShaderModule(vkDevice, vertShaderModule, nullptr);

    return new VulkanGraphicsPipeline(graphicsPipeline, pipelineLayout);
}

void VulkanPipelineFactory::destroyPipeline(GraphicsPipeline* pipeline, NativeDevice device) {
    if (!pipeline) return;

    auto* vkPipeline = static_cast<VulkanGraphicsPipeline*>(pipeline);
    VkDevice vkDevice = static_cast<VkDevice>(device);

    vkDestroyPipeline(vkDevice, vkPipeline->getVkPipeline(), nullptr);
    vkDestroyPipelineLayout(vkDevice, vkPipeline->getVkLayout(), nullptr);

    delete vkPipeline;
}

#endif // RENDER_API_VULKAN
