#include "Camera.hpp"

using namespace Scene;

Camera::Camera(const Loader::Camera& cam) : cameraToWorld_(cam.cameraToWorld)
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
