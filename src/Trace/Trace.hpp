#pragma once

#include "Trace/Camera.hpp"
#include "Trace/RenderBuffer.hpp"
#include "Trace/Scene.hpp"

class Trace
{
   private:
    RenderBuffer& imageBuffer_;
    const Camera& camera_;
    const Scene& scene_;

   public:
    Trace(RenderBuffer& imageBuffer, const Camera& camera, const Scene& scene) noexcept
        : imageBuffer_(imageBuffer), camera_(camera), scene_(scene) {};
    ~Trace() = default;

    void Render();
};
