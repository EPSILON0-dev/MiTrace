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

std::optional<RayHit> Mesh::Intersect(const Ray& ray, const glm::mat4& modelToWorld) const noexcept
{
    // Transform the ray to local space
    Ray localRay;
    auto invModelToWorld = glm::inverse(modelToWorld);
    localRay.origin = glm::vec3(invModelToWorld * glm::vec4(ray.origin, 1.0f));
    localRay.direction =
        glm::normalize(glm::vec3(invModelToWorld * glm::vec4(ray.direction, 0.0f)));

    float dist = std::numeric_limits<float>::max();
    glm::vec2 baryCoord;
    size_t triangleIndex;

    for (size_t i = 0; i < indices_.size(); i += 3)
    {
        const glm::vec3& v0 = positions_[indices_[i + 0]];
        const glm::vec3& v1 = positions_[indices_[i + 1]];
        const glm::vec3& v2 = positions_[indices_[i + 2]];

        float currentDistance;
        glm::vec2 currentBaryCoord;

        if (glm::intersectRayTriangle(
                localRay.origin, localRay.direction, v0, v1, v2, currentBaryCoord, currentDistance))
        {
            if (currentDistance > 0.0f && currentDistance < dist)
            {
                dist = currentDistance;
                baryCoord = currentBaryCoord;
                triangleIndex = i / 3;
            }
        }
    }

    if (dist < std::numeric_limits<float>::max())
    {
        RayHit hit;
        hit.worldPosition =
            glm::vec3(modelToWorld * glm::vec4(localRay.origin + dist * localRay.direction, 1.0f));
        hit.distance = glm::length(hit.worldPosition - ray.origin);
        hit.triangleIndex = triangleIndex;
        hit.baryCoord = baryCoord;

        return hit;
    }
    else
    {
        return std::nullopt;
    }
}
