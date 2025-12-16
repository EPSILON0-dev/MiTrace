#include "MeshInstance.hpp"

#include <limits>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/intersect.hpp>

#include "Intersect.hpp"

MeshInstance::MeshInstance(std::shared_ptr<Mesh> mesh, const glm::mat4& transform)
    : mesh_(std::move(mesh)), transform_(transform)
{
    // Compute world AABB
    glm::vec3 localMin = mesh_->GetAABBMin();
    glm::vec3 localMax = mesh_->GetAABBMax();

    glm::vec3 corners[8] = {
        {localMin.x, localMin.y, localMin.z},
        {localMin.x, localMin.y, localMax.z},
        {localMin.x, localMax.y, localMin.z},
        {localMin.x, localMax.y, localMax.z},
        {localMax.x, localMin.y, localMin.z},
        {localMax.x, localMin.y, localMax.z},
        {localMax.x, localMax.y, localMin.z},
        {localMax.x, localMax.y, localMax.z},
    };

    worldAABBMin_ = glm::vec3(std::numeric_limits<float>::max());
    worldAABBMax_ = glm::vec3(std::numeric_limits<float>::lowest());

    for (const auto& corner : corners)
    {
        glm::vec4 transformedCorner = transform_ * glm::vec4(corner, 1.0f);
        worldAABBMin_ = glm::min(worldAABBMin_, glm::vec3(transformedCorner));
        worldAABBMax_ = glm::max(worldAABBMax_, glm::vec3(transformedCorner));
    }
}

// TODO Move the intersect logic into Mesh class (only transform the ray into local space)
std::optional<RayHit> MeshInstance::IntersectRay(const Ray& ray) const
{
    if (!IntersectRayAABB(ray, worldAABBMin_, worldAABBMax_)) return std::nullopt;

    float closestDistance = std::numeric_limits<float>::max();
    size_t hitTriangleIndex = std::numeric_limits<size_t>::max();

    // Unfortunately a deadly sin has to be committed here: for loop over vertices.
    for (size_t i = 0; i < mesh_->GetIndices().size(); i += 3)
    {
        // TODO optimize: transform the ray into local space instead of transforming each vertex
        glm::vec4 v0 =
            transform_ * glm::vec4(mesh_->GetPositions()[mesh_->GetIndices()[i + 0]], 1.0f);
        glm::vec4 v1 =
            transform_ * glm::vec4(mesh_->GetPositions()[mesh_->GetIndices()[i + 1]], 1.0f);
        glm::vec4 v2 =
            transform_ * glm::vec4(mesh_->GetPositions()[mesh_->GetIndices()[i + 2]], 1.0f);

        glm::vec2 baryPosUnused;
        float distance;

        if (glm::intersectRayTriangle(ray.origin, ray.direction, glm::vec3(v0), glm::vec3(v1),
                glm::vec3(v2), baryPosUnused, distance))
        {
            if (distance < closestDistance)
            {
                closestDistance = distance;
                hitTriangleIndex = i / 3;
            }
        }
    }

    if (hitTriangleIndex == std::numeric_limits<size_t>::max()) return std::nullopt;

    return RayHit(ray, glm::vec3(0.0f), glm::vec3(0.0f), closestDistance);
}
