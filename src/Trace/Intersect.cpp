#include "Intersect.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/intersect.hpp>

bool IntersectRayAABB(const Ray& ray, const std::pair<glm::vec3, glm::vec3>& aabb)
{
    glm::vec3 invDir = 1.0f / ray.direction;
    glm::vec3 t1 = (aabb.first - ray.origin) * invDir;
    glm::vec3 t2 = (aabb.second - ray.origin) * invDir;
    glm::vec3 tmin3 = glm::min(t1, t2);
    glm::vec3 tmax3 = glm::max(t1, t2);

    return glm::compMax(tmin3) <= glm::compMin(tmax3);
}

static std::optional<RayHit> IntersectMeshInstanceInner(
    const Ray& ray, const Scene::MeshInstance& meshInstance) noexcept
{
    // Transform the ray to local space
    Ray localRay;
    auto modelToWorld = meshInstance.GetTransform();
    auto invModelToWorld = glm::inverse(modelToWorld);
    localRay.origin = glm::vec3(invModelToWorld * glm::vec4(ray.origin, 1.0f));
    localRay.direction =
        glm::normalize(glm::vec3(invModelToWorld * glm::vec4(ray.direction, 0.0f)));

    float dist = std::numeric_limits<float>::max();
    glm::vec2 baryCoord = glm::vec2(0.0f);
    size_t triangleIndex = 0;

    auto positions = meshInstance.GetMesh().GetPositions();
    auto indices = meshInstance.GetMesh().GetIndices();

    for (size_t i = 0; i < indices.size(); i += 3)
    {
        const glm::vec3& v0 = positions[indices[i + 0]];
        const glm::vec3& v1 = positions[indices[i + 1]];
        const glm::vec3& v2 = positions[indices[i + 2]];

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

std::optional<RayHit> IntersectMeshInstance(const Ray& ray, const Scene::MeshInstance& meshInstance)
{
    // First, check intersection with world AABB
    if (!IntersectRayAABB(ray, meshInstance.GetWorldAABB())) return std::nullopt;

    // Do the intersection test in local space
    auto hit = IntersectMeshInstanceInner(ray, meshInstance);
    if (hit.has_value())
    {
        hit->origin = ray.origin;
        hit->direction = ray.direction;
        hit->meshInstance = &meshInstance;
        return hit;
    }
    else
    {
        return std::nullopt;
    }
}