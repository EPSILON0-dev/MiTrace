#define GLM_ENABLE_EXPERIMENTAL
#include "Intersect.hpp"

#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/intersect.hpp>

#include "LocalStack.hpp"

static inline float IntersectRayAABB(const Ray& ray, const std::pair<glm::vec3, glm::vec3>& aabb)
{
    glm::vec3 invDir = 1.0f / ray.direction;
    glm::vec3 t1 = (aabb.first - ray.origin) * invDir;
    glm::vec3 t2 = (aabb.second - ray.origin) * invDir;
    glm::vec3 tmin3 = glm::min(t1, t2);
    glm::vec3 tmax3 = glm::max(t1, t2);

    return glm::compMax(tmin3) <= glm::compMin(tmax3) ? glm::compMin(tmax3) : -1.0f;
}

[[maybe_unused]] static void IntersectTrianglesLinear(const Ray& localRay,
    const Scene::MeshInstance& meshInstance, float& dist, glm::vec2& baryCoord,
    size_t& triangleIndex) noexcept
{
    auto positions = meshInstance.GetMesh().GetPositions();
    for (size_t i = 0; i < positions.size(); i += 3)
    {
        const glm::vec3& v0 = positions[i + 0];
        const glm::vec3& v1 = positions[i + 1];
        const glm::vec3& v2 = positions[i + 2];

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
}

[[maybe_unused]] static void IntersectTrianglesBVH(const Ray& localRay,
    const Scene::MeshInstance& meshInstance, float& dist, glm::vec2& baryCoord,
    size_t& triangleIndex) noexcept
{
    // Theoretically should be enough
    Stack<uint32_t, 64> stack;
    stack.Push(0);

    const auto& positions = meshInstance.GetMesh().GetPositions();
    const auto& bvh = meshInstance.GetMesh().GetBVH();

    while (!stack.Empty())
    {
        auto nodeIndex = stack.Pop();
        const auto& node = bvh.GetNodes()[nodeIndex];

        // If we missed completely, skip this node
        if (IntersectRayAABB(localRay, node.GetAABB()) < 0.0f) continue;

        // If it's a leaf, check the triangles
        if (node.IsLeaf())
        {
            auto index = node.GetTriangleIndex();
            auto count = node.GetTriangleCount();
            for (uint32_t i = index; i < index + count; i++)
            {
                auto currentTriangleIndex = bvh.GetIndices()[i];
                float currentDistance;
                glm::vec2 currentBaryCoord;

                const glm::vec3& v0 = positions[currentTriangleIndex * 3 + 0];
                const glm::vec3& v1 = positions[currentTriangleIndex * 3 + 1];
                const glm::vec3& v2 = positions[currentTriangleIndex * 3 + 2];
                if (glm::intersectRayTriangle(localRay.origin, localRay.direction, v0, v1, v2,
                        currentBaryCoord, currentDistance))
                {
                    if (currentDistance > 0.0f && currentDistance < dist)
                    {
                        dist = currentDistance;
                        baryCoord = currentBaryCoord;
                        triangleIndex = currentTriangleIndex;
                    }
                }
            }
        }

        // Else keep descending
        else
        {
            stack.Push(node.GetChildA());
            stack.Push(node.GetChildB());
        }
    }
}

std::optional<RayHit> IntersectMeshInstance(
    const Ray& ray, const Scene::MeshInstance& meshInstance) noexcept
{
    // First, check intersection with world AABB
    if (IntersectRayAABB(ray, meshInstance.GetWorldAABB()) < 0.0f) return std::nullopt;

    // Transform the ray to local space
    Ray localRay;
    const auto& modelToWorld = meshInstance.GetTransform();
    const auto invModelToWorld = glm::inverse(modelToWorld);
    localRay.origin = glm::vec3(invModelToWorld * glm::vec4(ray.origin, 1.0f));
    localRay.direction =
        glm::normalize(glm::vec3(invModelToWorld * glm::vec4(ray.direction, 0.0f)));

    // Find the closest triangle intersection
    float dist = std::numeric_limits<float>::max();
    auto baryCoord = glm::vec2(0.0f);
    size_t triangleIndex = 0;
    if (meshInstance.GetMesh().HasBVH())
    {
        IntersectTrianglesBVH(localRay, meshInstance, dist, baryCoord, triangleIndex);
    }
    else
    {
        IntersectTrianglesLinear(localRay, meshInstance, dist, baryCoord, triangleIndex);
    }

    // If there is an intersection, transform the hit position back to world space and return the
    // hit info
    if (dist < std::numeric_limits<float>::max())
    {
        RayHit hit;
        hit.worldPosition =
            glm::vec3(modelToWorld * glm::vec4(localRay.origin + dist * localRay.direction, 1.0f));
        hit.distance = glm::length(hit.worldPosition - ray.origin);
        hit.triangleIndex = triangleIndex;
        hit.baryCoord = baryCoord;

        hit.origin = ray.origin;
        hit.direction = ray.direction;
        hit.meshInstance = &meshInstance;

        return hit;
    }
    else
    {
        return std::nullopt;
    }
}
