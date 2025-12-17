#include "Trace.hpp"

#include <glm/fwd.hpp>

#include "Trace/MaterialBase.hpp"

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

    const auto uv0 = mesh.GetTexCoord0()[mesh.GetIndices()[bestHit->triangleIndex * 3 + 0]];
    const auto uv1 = mesh.GetTexCoord0()[mesh.GetIndices()[bestHit->triangleIndex * 3 + 1]];
    const auto uv2 = mesh.GetTexCoord0()[mesh.GetIndices()[bestHit->triangleIndex * 3 + 2]];

    glm::vec2 interpolatedUV = bestHit->baryCoord.x * uv0 + bestHit->baryCoord.y * uv1 +
                               (1.0f - bestHit->baryCoord.x - bestHit->baryCoord.y) * uv2;

    glm::vec3 unused, color(1.0f);
    MaterialBase::GeometryInfo geomInfo{
        .normal = glm::vec3(0.0f, 0.0f, 0.0f),   // Placeholder
        .tangent = glm::vec3(0.0f, 0.0f, 0.0f),  // Placeholder
        .uv = interpolatedUV,
    };
    material->Reflect(geomInfo, unused, color);

    return glm::u8vec4(glm::clamp(color * 255.0f, glm::vec3(0.0f), glm::vec3(255.0f)), 255);
}
