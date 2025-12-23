#ifndef MY_APPLICATION_RENDEREROPENGL_H
#define MY_APPLICATION_RENDEREROPENGL_H

#include "RendererAPI.h"
#include <EGL/egl.h>
#include <memory>
#include <vector>
#include "Model.h"
#include "ShaderOpenGL.h"

struct android_app;

class RendererOpenGL : public RendererAPI {
public:
    RendererOpenGL(android_app *pApp);
    virtual ~RendererOpenGL();

    void init() override;
    void render() override;
    void onConfigChanged() override;  // 空实现，OpenGL 不需要特殊处理

private:
    void updateRenderArea();
    void createModels();

    android_app *app_;
    EGLDisplay display_;
    EGLSurface surface_;
    EGLContext context_;
    EGLint width_;
    EGLint height_;

    bool shaderNeedsNewProjectionMatrix_;

    std::unique_ptr<ShaderOpenGL> shader_;
    std::vector<Model> models_;
};

#endif //MY_APPLICATION_RENDEREROPENGL_H