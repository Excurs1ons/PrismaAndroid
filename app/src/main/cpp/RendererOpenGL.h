#ifndef MY_APPLICATION_RENDEREROPENGL_H
#define MY_APPLICATION_RENDEREROPENGL_H

#include "RendererAPI.h"
#include <EGL/egl.h>
#include <memory>
#include <vector>
#include "Model.h"
#include "ShaderOpenGL.h"


#include "graphic/RenderAPI.h"
#include "Platform.h"
#include "graphic/interfaces/RenderTypes.h"
using namespace PrismaEngine::Graphic;
struct android_app;

class RendererOpenGL :public RendererAPI// , public PrismaEngine::Graphic::IRenderDevice
        {
public:
    RendererOpenGL(android_app *pApp);
    RendererOpenGL(WindowHandle *pApp);
    ~RendererOpenGL() override;

    [[nodiscard]] std::string GetName() const;// override;
    void init();
    void onConfigChanged();
    bool Initialize(const DeviceDesc& desc);// override;
    void Render();

    void render() override;


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