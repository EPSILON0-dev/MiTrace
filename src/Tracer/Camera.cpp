#include "Camera.hpp"

#include <spdlog/spdlog.h>

using namespace Tracer;

void Camera::CheckCameraAspectRatio(float renderAspectRatio) const noexcept
{
    if (fabsf(GetAspectRatio() - renderAspectRatio) < 0.01f) return;

    spdlog::warn(
        "Camera aspect ratio ({}) does not match render aspect ratio ({}). This may lead to "
        "distorted renders.",
        GetAspectRatio(), renderAspectRatio);
}

Ray Camera::GeneratePerspectiveRay(float u, float v, float aspectRatio) const noexcept
{
    float fovScale = tanf(GetFov() * 0.5f);
    float px = (2.0f * u - 1.0f) * fovScale * aspectRatio;
    float py = (2.0f * v - 1.0f) * fovScale;

    glm::vec4 rayOriginCameraSpace = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    glm::vec4 rayDirectionCameraSpace = glm::normalize(glm::vec4(px, -py, -1.0f, 0.0f));

    glm::vec4 rayOriginWorldSpace = GetCameraToWorld() * rayOriginCameraSpace;
    glm::vec4 rayDirectionWorldSpace = GetCameraToWorld() * rayDirectionCameraSpace;

    return {glm::vec3(rayOriginWorldSpace), glm::normalize(glm::vec3(rayDirectionWorldSpace))};
}

Ray Camera::GenerateOrthogonalRay(float u, float v, float aspectRatio) const noexcept
{
    float px = (2.0f * u - 1.0f) * GetOrthogonalSize().x * aspectRatio / GetAspectRatio();
    float py = (2.0f * v - 1.0f) * GetOrthogonalSize().y;

    glm::vec4 rayOriginCameraSpace = glm::vec4(px, -py, 0.0f, 1.0f);
    glm::vec4 rayDirectionCameraSpace = glm::normalize(glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));

    glm::vec4 rayOriginWorldSpace = GetCameraToWorld() * rayOriginCameraSpace;
    glm::vec4 rayDirectionWorldSpace = GetCameraToWorld() * rayDirectionCameraSpace;

    return {glm::vec3(rayOriginWorldSpace), glm::normalize(glm::vec3(rayDirectionWorldSpace))};
}

Ray Camera::GenerateCameraRay(float u, float v, float aspectRatio) const noexcept
{
    return GetType() == Scene::Camera::Type::Perspective ? GeneratePerspectiveRay(u, v, aspectRatio)
                                                         : GenerateOrthogonalRay(u, v, aspectRatio);
}
