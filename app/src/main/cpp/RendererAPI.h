#ifndef MY_APPLICATION_RENDERERAPI_H
#define MY_APPLICATION_RENDERERAPI_H

class RendererAPI {
public:
    virtual ~RendererAPI() = default;
    virtual void init() = 0;
    virtual void render() = 0;
};

#endif //MY_APPLICATION_RENDERERAPI_H