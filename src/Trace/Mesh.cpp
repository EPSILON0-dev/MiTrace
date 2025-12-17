#include "Mesh.hpp"

#include <numeric>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/intersect.hpp>

glm::vec3 Mesh::GetAABBMin() const noexcept
{
    return std::accumulate(positions_.begin(), positions_.end(),
        glm::vec3(std::numeric_limits<float>::max()),
        [](const auto& a, const auto& b) { return glm::min(a, b); });
}

glm::vec3 Mesh::GetAABBMax() const noexcept
{
    return std::accumulate(positions_.begin(), positions_.end(),
        glm::vec3(std::numeric_limits<float>::lowest()),
        [](const auto& a, const auto& b) { return glm::max(a, b); });
}

bool Mesh::Intersect(
    const Ray& ray, size_t& triangleIndex, float& dist, glm::vec2& baryCoord) const noexcept
{
    triangleIndex = std::numeric_limits<size_t>::max();
    dist = std::numeric_limits<float>::max();

    for (size_t i = 0; i < indices_.size(); i += 3)
    {
        const glm::vec3& v0 = positions_[indices_[i + 0]];
        const glm::vec3& v1 = positions_[indices_[i + 1]];
        const glm::vec3& v2 = positions_[indices_[i + 2]];

        float currentDistance;
        glm::vec2 currentBaryCoord;

        if (glm::intersectRayTriangle(ray.origin, ray.direction, glm::vec3(v0), glm::vec3(v1),
                glm::vec3(v2), currentBaryCoord, currentDistance))
        {
            if (currentDistance < dist)
            {
                dist = currentDistance;
                baryCoord = currentBaryCoord;
                triangleIndex = i / 3;
            }
        }
    }

    return triangleIndex != std::numeric_limits<size_t>::max();
}
