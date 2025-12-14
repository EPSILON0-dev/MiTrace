#include "camera.hpp"

Ray Camera::GenerateRay(float u, float v, float aspectRatio) const noexcept
{
    // --- AI GENERATED CODE START ---

    // Convert field of view from degrees to radians
    float theta = glm::radians(yfov);
    float halfHeight = tan(theta / 2.0f);
    float halfWidth = aspectRatio * halfHeight;

    // Calculate the camera basis vectors
    glm::vec3 w = glm::normalize(-forward); // Camera looks towards -forward
    glm::vec3 uVec = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), w));
    glm::vec3 vVec = glm::cross(w, uVec);

    // Calculate the lower left corner of the view plane
    glm::vec3 lowerLeftCorner = position - halfWidth * uVec - halfHeight * vVec - w;

    // Calculate the horizontal and vertical spans of the view plane
    glm::vec3 horizontal = 2.0f * halfWidth * uVec;
    glm::vec3 vertical = 2.0f * halfHeight * vVec;

    // Calculate the direction of the ray through the pixel (u, v)
    glm::vec3 rayDirection = lowerLeftCorner + u * horizontal + v * vertical - position;

    return Ray(position, glm::normalize(rayDirection));

    // --- AI GENERATED CODE END ---
}
