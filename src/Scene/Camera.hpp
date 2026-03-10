#pragma once

#include <glm/glm.hpp>

#include "Loader/Types.hpp"

namespace Scene
{

class Camera
{
   public:
    enum class Type : uint8_t
    {
        Perspective,
        Orthogonal
    };

   private:
    glm::mat4 cameraToWorld_;
    glm::vec2 orthogonalSize_;
    float perspectiveFov_;
    float aspectRatio_;
    Type type_;

   public:
    Camera() noexcept : perspectiveFov_(glm::radians(60.0f)) {};

    Camera(const Loader::Camera& cam) : cameraToWorld_(cam.cameraToWorld)
    {
        if (cam.type == Loader::CameraType::Orthogonal)
        {
            type_ = Type::Orthogonal;
            orthogonalSize_ = cam.orthogonalMag;
            perspectiveFov_ = 0.0f;
            aspectRatio_ = cam.aspectRatio;
        }

        else if (cam.type == Loader::CameraType::Perspective)
        {
            type_ = Type::Perspective;
            perspectiveFov_ = cam.perspectiveFov;
            orthogonalSize_ = glm::vec2(0.0f);
            aspectRatio_ = cam.aspectRatio;
        }

        else
        {
            throw std::runtime_error("Unknown camera type");
        }
    };

   public:
    auto GetType() const noexcept { return type_; }
    auto GetFov() const noexcept { return perspectiveFov_; }
    auto GetAspectRatio() const noexcept { return aspectRatio_; }
    const auto& GetCameraToWorld() const noexcept { return cameraToWorld_; }
    const auto& GetOrthogonalSize() const noexcept { return orthogonalSize_; }
};

}  // namespace Scene
