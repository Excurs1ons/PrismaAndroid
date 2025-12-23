#ifndef MY_APPLICATION_RENDERERAPI_H
#define MY_APPLICATION_RENDERERAPI_H

class RendererAPI {
public:
    virtual ~RendererAPI() = default;
    virtual void init() = 0;
    virtual void render() = 0;
    virtual void onConfigChanged() = 0;  // 处理屏幕旋转等配置变化
};

#endif //MY_APPLICATION_RENDERERAPI_H