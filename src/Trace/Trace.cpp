#include "Trace/Trace.hpp"

#include <glm/fwd.hpp>

#include "Trace/MaterialGLTF.hpp"
#include "Trace/RayHit.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/intersect.hpp>

static std::optional<RayHit> TraceScene(
    const Ray& ray, const Scene& scene, bool anyHit = false) noexcept
{
    float lowestDistance = std::numeric_limits<float>::max();
    std::optional<RayHit> bestHit = std::nullopt;

    for (const auto& meshInstance : scene.GetMeshInstances())
    {
        if (const auto hit = meshInstance.IntersectRay(ray); hit.has_value())
        {
            if (hit->distance < lowestDistance)
            {
                lowestDistance = hit->distance;
                bestHit = *hit;
                if (anyHit) break;
            }
        }
    }

    return bestHit;
}

void Trace::Render()
{
    const auto imageWidth = imageBuffer_.GetWidth();
    const auto imageHeight = imageBuffer_.GetHeight();

    for (unsigned y = 0; y < imageHeight; ++y)
    {
        for (unsigned x = 0; x < imageWidth; ++x)
        {
            const glm::vec2 uv{
                (static_cast<float>(x) + 0.5f) / imageWidth,
                (static_cast<float>(y) + 0.5f) / imageHeight,
            };
            const auto aspectRatio = static_cast<float>(imageWidth) / imageHeight;

            const Ray ray = camera_.GenerateRay(uv.x, uv.y, aspectRatio);

            glm::vec3 pixelColor{0.0f, 0.0f, 0.0f};

            if (const auto hit = TraceScene(ray, scene_); hit.has_value())
            {
                const auto& matBase = hit->meshInstance->GetMesh().GetMaterial().get();
                const auto& mat = dynamic_cast<MaterialGLTF*>(matBase);
                const auto geom = RayHitGeometryInfo(*hit);
                const auto uv = geom.TexCoord0;
                pixelColor = mat->GetBaseColor(uv);
            }
            else
            {
                pixelColor = glm::vec3{0.0f, 0.0f, 0.0f};
            }

            imageBuffer_.SetPixel(x, y, pixelColor);
        }
    }
}
