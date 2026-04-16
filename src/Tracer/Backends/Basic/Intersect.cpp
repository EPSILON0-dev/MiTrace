#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/intersect.hpp>

#include "Intersect.hpp"

using namespace BasicBackend;

static inline float IntersectRayAABB(const Ray& ray, const std::pair<glm::vec3, glm::vec3>& aabb)
{
    glm::vec3 invDir = 1.0f / ray.direction;
    glm::vec3 t1 = (aabb.first - ray.origin) * invDir;
    glm::vec3 t2 = (aabb.second - ray.origin) * invDir;
    glm::vec3 tmin3 = glm::min(t1, t2);
    glm::vec3 tmax3 = glm::max(t1, t2);

    return glm::compMax(tmin3) <= glm::compMin(tmax3) ? glm::compMin(tmax3) : -1.0f;
}

static void IntersectTrianglesLinear(const Ray& localRay, const Scene::MeshInstance& meshInstance,
    float& dist, glm::vec2& baryCoord, size_t& triangleIndex) noexcept
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

static void IntersectTrianglesBVH(const Ray& localRay, const Scene::MeshInstance& meshInstance,
    float& dist, glm::vec2& baryCoord, size_t& triangleIndex) noexcept
{
    struct alignas(8) StackEntry
    {
        uint32_t nodeIndex;
        float dist;
    };
    static thread_local std::vector<StackEntry> stack;

    const float minDelta = 0.1f;
    auto rootNode = meshInstance.GetMesh().GetBVH().GetNodes()[0];
    auto rootDist = IntersectRayAABB(localRay, rootNode.GetAABB());
    stack.clear();
    stack.push_back({0, rootDist});

    const auto& positions = meshInstance.GetMesh().GetPositions();
    const auto& bvh = meshInstance.GetMesh().GetBVH();

    while (!stack.empty())
    {
        auto entry = stack.back();
        stack.pop_back();
        const auto& node = bvh.GetNodes()[entry.nodeIndex];
        const auto nodeDist = entry.dist;

        // Skip this node if already got a closer hit
        // NOTE: There's no need to check for miss here
        if (nodeDist > dist + minDelta) continue;

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
            auto& childA = bvh.GetNodes()[node.GetChildA()];
            auto& childB = bvh.GetNodes()[node.GetChildB()];
            auto childADist = IntersectRayAABB(localRay, childA.GetAABB());
            auto childBDist = IntersectRayAABB(localRay, childB.GetAABB());
            auto pushA = (childADist >= 0.0f && childADist <= dist + minDelta);
            auto pushB = (childBDist >= 0.0f && childBDist <= dist + minDelta);
            if (pushA && pushB)
            {
                // Push the closer one first
                if (childADist < childBDist)
                {
                    stack.push_back({node.GetChildB(), childBDist});
                    stack.push_back({node.GetChildA(), childADist});
                }
                else
                {
                    stack.push_back({node.GetChildA(), childADist});
                    stack.push_back({node.GetChildB(), childBDist});
                }
            }
            else if (pushA)
            {
                stack.push_back({node.GetChildA(), childADist});
            }
            else if (pushB)
            {
                stack.push_back({node.GetChildB(), childBDist});
            }
        }
    }
}

static std::optional<RayHit> IntersectMeshInstance(
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

std::optional<RayHit> BasicBackend::IntersectScene(
    const Ray& ray, const Scene::Scene& scene) noexcept
{
    float lowestDistance = std::numeric_limits<float>::max();
    std::optional<RayHit> bestHit = std::nullopt;

    for (const auto& meshInstance : scene.GetMeshInstances())
    {
        if (const auto hit = IntersectMeshInstance(ray, meshInstance); hit.has_value())
        {
            if (hit->distance < lowestDistance)
            {
                lowestDistance = hit->distance;
                bestHit = *hit;
            }
        }
    }

    return bestHit;
}
