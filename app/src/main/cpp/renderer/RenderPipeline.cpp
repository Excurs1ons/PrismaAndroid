#include "RenderPipeline.h"
#include "RenderPass.h"
#include "OpaquePass.h"
#include "BackgroundPass.h"
#include "../AndroidOut.h"
#include <stdexcept>

void RenderPipeline::addPass(std::unique_ptr<RenderPass> pass) {
    if (!pass)
    {
        aout << "无效的Pass" << std::endl;
        return;
    }
    aout << "已添加Pass:" << std::string(pass->name) << std::endl;
    passes_.push_back(std::move(pass));
    //move之后原引用置空
}

void RenderPipeline::initialize(VkDevice device, VkRenderPass apiRenderPass) {
    device_ = device;
    apiRenderPass_ = apiRenderPass;

    // 初始化所有 Pass
    for (auto& pass : passes_) {
        pass->initialize(device, apiRenderPass);
    }
}

void RenderPipeline::execute(VkCommandBuffer cmdBuffer) {
    // 按顺序执行所有 Pass
    for (auto& pass : passes_) {
        pass->record(cmdBuffer);
    }
}

void RenderPipeline::setCurrentFrame(uint32_t currentFrame) {
    // 传递当前帧索引给所有 Pass
    for (auto& pass : passes_) {
        auto* opaquePass = dynamic_cast<OpaquePass*>(pass.get());
        if (opaquePass) {
            opaquePass->setCurrentFrame(currentFrame);
        }
        auto* backgroundPass = dynamic_cast<BackgroundPass*>(pass.get());
        if (backgroundPass) {
            backgroundPass->setCurrentFrame(currentFrame);
        }
    }
}

OpaquePass* RenderPipeline::getOpaquePass() {
    for (auto& pass : passes_) {
        auto* opaquePass = dynamic_cast<OpaquePass*>(pass.get());
        if (opaquePass) {
            return opaquePass;
        }
    }
    return nullptr;
}

BackgroundPass* RenderPipeline::getBackgroundPass() {
    for (auto& pass : passes_) {
        auto* backgroundPass = dynamic_cast<BackgroundPass*>(pass.get());
        if (backgroundPass) {
            return backgroundPass;
        }
    }
    return nullptr;
}

void RenderPipeline::cleanup(VkDevice device) {
    // 清理所有 Pass（按相反顺序）
    for (auto it = passes_.rbegin(); it != passes_.rend(); ++it) {
        (*it)->cleanup(device);
    }
    passes_.clear();
}
