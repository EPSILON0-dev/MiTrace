#pragma once

#include "Scene/Camera.hpp"
#include "Ray.hpp"

namespace BasicBackend
{

class BasicCamera : public Scene::Camera
{
   public:
    BasicCamera(const Scene::Camera& cam) : Scene::Camera(cam) {}

    void CheckCameraAspectRatio(float renderAspectRatio) const noexcept;
    Ray GeneratePerspectiveRay(float u, float v, float aspectRatio) const noexcept;
    Ray GenerateOrthogonalRay(float u, float v, float aspectRatio) const noexcept;
    Ray GenerateCameraRay(float u, float v, float aspectRatio) const noexcept;
};

}  // namespace BasicBackend
