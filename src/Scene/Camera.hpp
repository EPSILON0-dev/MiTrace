#pragma once

#include <glm/glm.hpp>

#include "Loader/Types.hpp"

namespace Scene
{

class Camera
{
   private:
    glm::mat4 cameraToWorld_;
    float yfovRadians_;

   public:
    Camera() noexcept : yfovRadians_(glm::radians(60.0f)) {};

    Camera(const Loader::Camera& cam)
        : cameraToWorld_(cam.cameraToWorld), yfovRadians_(cam.perspectiveFovY)
    {
        if (cam.type == Loader::CameraType::Orthogonal)
            throw std::runtime_error("Orthogonal camera not implemented");
    };

   public:
    inline glm::mat4 GetCameraToWorld() const noexcept { return cameraToWorld_; }
    inline float GetYFovRadians() const noexcept { return yfovRadians_; }
};

}  // namespace Scene
