#include "Trace.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/intersect.hpp>

static bool RaySphereIntersect(const Ray& ray, const glm::vec4& sphere, float& distance)
{
    return glm::intersectRaySphere(
        ray.origin, ray.direction, glm::vec3(sphere), sphere.w * sphere.w, distance);
}

bool Trace::TraceScene(const Ray& ray, const Scene& scene)
{
    float lowestDistance = std::numeric_limits<float>::max();
    bool didHit = false;

    for (const auto& meshInstance : scene.GetMeshInstances())
    {
        auto hit = meshInstance.IntersectRay(ray);
        if (hit.has_value())
        {
            didHit = true;
            if (hit->distance < lowestDistance)
                lowestDistance = hit->distance;
        }
    }

    return didHit;
}
