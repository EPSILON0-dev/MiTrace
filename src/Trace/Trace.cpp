#include "Trace.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/intersect.hpp>
#include <numeric>

static bool RaySphereIntersect(const Ray& ray, const glm::vec4& sphere, float& distance)
{
    return glm::intersectRaySphere(
        ray.origin, ray.direction, glm::vec3(sphere), sphere.w * sphere.w, distance);
}

bool Trace::TraceScene(const Ray& ray, const Scene& scene)
{
    RayHit closestHit;

    auto checkLambda = [&ray](auto closestHit, const auto& sphere)
    {
        float hitDist;
        if (RaySphereIntersect(ray, sphere, hitDist))
            if (hitDist < closestHit.distance || closestHit.distance < 0.0f)
                closestHit.distance = hitDist;
        return closestHit;
    };

    closestHit = std::accumulate(
        scene.GetSpheres().begin(), scene.GetSpheres().end(), closestHit, checkLambda);

    return closestHit.distance > 0.0f;
}
