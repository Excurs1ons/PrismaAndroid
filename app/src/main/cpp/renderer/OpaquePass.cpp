#include "OpaquePass.h"
#include "../ShaderVulkan.h"
#include "../Model.h"
#include "../MeshRenderer.h"
#include "../Scene.h"
#include "../GameObject.h"
#include "../AndroidOut.h"
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <array>
#include <stdexcept>
#include <cstring>

// UBO 结构（与 RendererVulkan.cpp 中定义的一致）
struct UniformBufferObject {
    alignas(16) Matrix4 model;
    alignas(16) Matrix4 view;
    alignas(16) Matrix4 proj;
};

void OpaquePass::setSwapChainExtent(VkExtent2D extent) {
    swapChainExtent_ = extent;
}

void OpaquePass::setAndroidApp(android_app* app) {
    app_ = app;
}

void OpaquePass::setScene(Scene* scene) {
    scene_ = scene;
}

void OpaquePass::initialize(VkDevice device, VkRenderPass renderPass) {
    createPipeline(device, renderPass);
}

void OpaquePass::createPipeline(VkDevice device, VkRenderPass renderPass) {
    if (!app_) {
        throw std::runtime_error("OpaquePass::createPipeline: android_app not set!");
    }

    auto vertShaderCode = ShaderVulkan::loadShader(app_->activity->assetManager, "shaders/shader.vert.spv");
    auto fragShaderCode = ShaderVulkan::loadShader(app_->activity->assetManager, "shaders/shader.frag.spv");

    if (vertShaderCode.empty() || fragShaderCode.empty()) {
        throw std::runtime_error("Failed to load shader files!");
    }

    VkShaderModule vertShaderModule;
    VkShaderModuleCreateInfo vertCreateInfo{};
    vertCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertCreateInfo.codeSize = vertShaderCode.size() * 4;
    vertCreateInfo.pCode = vertShaderCode.data();
    if (vkCreateShaderModule(device, &vertCreateInfo, nullptr, &vertShaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create vertex shader module!");
    }

    VkShaderModule fragShaderModule;
    VkShaderModuleCreateInfo fragCreateInfo{};
    fragCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragCreateInfo.codeSize = fragShaderCode.size() * 4;
    fragCreateInfo.pCode = fragShaderCode.data();
    if (vkCreateShaderModule(device, &fragCreateInfo, nullptr, &fragShaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create fragment shader module!");
    }

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

    // Viewport 和 Scissor
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapChainExtent_.width;
    viewport.height = (float) swapChainExtent_.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent_;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // 光栅化状态
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;  // 双面渲染
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

    // Pipeline Layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout_;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS) {
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
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = pipelineLayout_;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }

    // 清理着色器模块
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);

    aout << "OpaquePass: Graphics pipeline created successfully." << std::endl;
}

// TODO: 从 RendererVulkan::recordCommandBuffer() 中迁移渲染代码
void OpaquePass::record(VkCommandBuffer cmdBuffer) {
    if (!scene_) {
        return;
    }

    // 设置 Viewport 和 Scissor
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapChainExtent_.width;
    viewport.height = (float) swapChainExtent_.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent_;
    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

    // 绑定 Graphics Pipeline
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

    // 获取所有 MeshRenderer 对象
    auto& gameObjects = scene_->getGameObjects();
    std::vector<size_t> meshRendererIndices;

    for (size_t i = 0; i < gameObjects.size(); i++) {
        auto go = gameObjects[i];
        if (go->getComponent<MeshRenderer>()) {
            meshRendererIndices.push_back(i);
        }
    }

    // 渲染所有 MeshRenderer 对象
    for (size_t j = 0; j < meshRendererIndices.size(); j++) {
        size_t i = meshRendererIndices[j];
        auto go = gameObjects[i];
        auto meshRenderer = go->getComponent<MeshRenderer>();
        auto model = meshRenderer->getModel();

        VkBuffer vertexBuffers[] = {renderObjects_[j].vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, vertexBuffers, offsets);

        vkCmdBindIndexBuffer(cmdBuffer, renderObjects_[j].indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                               pipelineLayout_, 0, 1,
                               &renderObjects_[j].descriptorSets[currentFrame_], 0, nullptr);

        vkCmdDrawIndexed(cmdBuffer, static_cast<uint32_t>(model->getIndexCount()), 1, 0, 0, 0);
    }
}

// 清理资源
void OpaquePass::cleanup(VkDevice device) {
    if (pipelineLayout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, pipelineLayout_, nullptr);
        pipelineLayout_ = VK_NULL_HANDLE;
    }
    if (pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, pipeline_, nullptr);
        pipeline_ = VK_NULL_HANDLE;
    }
}

void OpaquePass::addRenderObject(const RenderObjectData& object) {
    renderObjects_.push_back(object);
}

void OpaquePass::setCurrentFrame(uint32_t currentFrame) {
    currentFrame_ = currentFrame;
}

void OpaquePass::setDescriptorSetLayout(VkDescriptorSetLayout layout) {
    descriptorSetLayout_ = layout;
}

void OpaquePass::updateUniformBuffer(const std::vector<std::shared_ptr<GameObject>>& gameObjects,
                                     const Matrix4& view, const Matrix4& proj,
                                     uint32_t currentImage, float time) {
    // 获取所有 MeshRenderer 对象
    std::vector<size_t> meshRendererIndices;
    for (size_t i = 0; i < gameObjects.size(); i++) {
        auto go = gameObjects[i];
        if (go->getComponent<MeshRenderer>()) {
            meshRendererIndices.push_back(i);
        }
    }

    // 更新每个 MeshRenderer 的 UBO
    for (size_t j = 0; j < meshRendererIndices.size(); j++) {
        size_t i = meshRendererIndices[j];
        auto go = gameObjects[i];

        // 更新立方体旋转动画
        if (go->name == "Cube") {
            go->rotation.x = time * 30.0f;
            go->rotation.y = time * 30.0f;
        }

        UniformBufferObject ubo{};
        ubo.model = go->getTransformMatrix();
        ubo.view = view;
        ubo.proj = proj;

        if (j < renderObjects_.size() && currentImage < renderObjects_[j].uniformBuffersMapped.size()) {
            memcpy(renderObjects_[j].uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
        }
    }
}


