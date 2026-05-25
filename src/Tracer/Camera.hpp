#pragma once

#include "Ray.hpp"
#include "Scene/Camera.hpp"

namespace Tracer
{

class Camera : public Scene::Camera
{
   public:
    Camera(const Scene::Camera& cam) : Scene::Camera(cam) {}

    void CheckCameraAspectRatio(float renderAspectRatio) const noexcept;
    Ray GeneratePerspectiveRay(float u, float v, float aspectRatio) const noexcept;
    Ray GenerateOrthogonalRay(float u, float v, float aspectRatio) const noexcept;
    Ray GenerateCameraRay(float u, float v, float aspectRatio) const noexcept;
};

}  // namespace Tracer
