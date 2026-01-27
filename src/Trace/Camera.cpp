#include "Camera.hpp"

Ray Camera::GenerateRay(float u, float v, float aspectRatio) const noexcept
{
    float fovScale = tanf(yfovRadians_ * 0.5f);
    float px = (2.0f * u - 1.0f) * fovScale * aspectRatio;
    float py = (2.0f * v - 1.0f) * fovScale;

    glm::vec4 rayOriginCameraSpace = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    glm::vec4 rayDirectionCameraSpace = glm::normalize(glm::vec4(px, -py, -1.0f, 0.0f));

    glm::vec4 rayOriginWorldSpace = cameraToWorld_ * rayOriginCameraSpace;
    glm::vec4 rayDirectionWorldSpace = cameraToWorld_ * rayDirectionCameraSpace;

    return Ray(glm::vec3(rayOriginWorldSpace), glm::normalize(glm::vec3(rayDirectionWorldSpace)));
}
