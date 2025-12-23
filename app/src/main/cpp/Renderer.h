#ifndef ANDROIDGLINVESTIGATIONS_RENDERER_H
#define ANDROIDGLINVESTIGATIONS_RENDERER_H

#include <memory>
#include "RendererAPI.h"

struct android_app;

class Renderer {
public:
    /*!
     * @param pApp the android_app this Renderer belongs to, needed to configure GL
     */
    Renderer(android_app *pApp);

    virtual ~Renderer();

    /*!
     * Handles input from the android_app.
     *
     * Note: this will clear the input queue
     */
    void handleInput();

    /*!
     * Renders all the models in the renderer
     */
    void render();

    /*!
     * Handles configuration changes (e.g., screen rotation)
     */
    void onConfigChanged();

private:
    android_app *app_;
    std::unique_ptr<RendererAPI> impl_;
};

#endif //ANDROIDGLINVESTIGATIONS_RENDERER_H