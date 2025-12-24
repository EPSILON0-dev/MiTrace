#include "Trace.hpp"

#include <glm/fwd.hpp>

#include "Trace/RayHit.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/intersect.hpp>

glm::u8vec4 Trace::TraceScene(const Ray& ray, const Scene& scene)
{
    float lowestDistance = std::numeric_limits<float>::max();
    std::optional<RayHit> bestHit = std::nullopt;

    // Find the closest triangle
    for (const auto& meshInstance : scene.GetMeshInstances())
    {
        if (const auto hit = meshInstance.IntersectRay(ray); hit.has_value())
        {
            // FIXME : Distance in world space should be compared, the intersect method returns
            // distance in model's local space
            if (hit->distance < lowestDistance)
            {
                lowestDistance = hit->distance;
                bestHit = *hit;
            }
        }
    }

    // Return if we missed completely
    if (!bestHit.has_value())
    {
        return glm::u8vec4(0, 0, 0, 255);
    }

    const auto& mesh = bestHit->meshInstance->GetMesh();
    const auto& material = mesh.GetMaterial();
    RayHitGeometryInfo geomInfo(*bestHit);

    glm::vec3 unused, color(1.0f);
    material->Reflect(geomInfo, unused, color);

    const auto hitPos = ray.origin + ray.direction * bestHit->distance;
    const auto lightPos = static_cast<glm::vec3>(scene.GetLights()[0].GetTransform()[3]);
    const auto lightDir = lightPos - hitPos;

    bool inShadow = false;
    Ray shadowRay(hitPos - ray.direction * 0.001f, glm::normalize(lightDir));
    const float lightDistance = glm::length(lightDir);
    for (const auto& meshInstance : scene.GetMeshInstances())
    {
        if (const auto shadowHit = meshInstance.IntersectRay(shadowRay); shadowHit.has_value())
        {
            if (shadowHit->distance < lightDistance)
            {
                inShadow = true;
                break;
            }
        }
    }

    const float dot = glm::max(glm::dot(glm::normalize(lightDir), geomInfo.Normal), 0.0f);
    color *= (inShadow ? 0.0f : 0.7f * dot) + 0.3f;
    return glm::u8vec4(glm::clamp(color * 255.0f, glm::vec3(0.0f), glm::vec3(255.0f)), 255);
}
