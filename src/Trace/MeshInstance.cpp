#include "MeshInstance.hpp"

#include <limits>

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

std::optional<RayHit> MeshInstance::IntersectRay(const Ray& ray) const
{
    // First, check intersection with world AABB
    if (!IntersectRayAABB(ray, worldAABBMin_, worldAABBMax_)) return std::nullopt;

    // Do the intersection test in local space
    auto hit = mesh_->Intersect(ray, transform_);
    if (hit.has_value())
    {
        hit->origin = ray.origin;
        hit->direction = ray.direction;
        hit->meshInstance = this;
        return hit;
    }
    else
    {
        return std::nullopt;
    }
}
