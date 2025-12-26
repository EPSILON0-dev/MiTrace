#include "Trace.hpp"

#include <glm/fwd.hpp>

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

glm::u8vec4 Trace::TraceSample(const Ray& ray, const Scene& scene)
{
    // TODO don't do this for every sample...
    std::random_device rd;
    std::mt19937 rng(rd());

    const int maxBounces = 3;
    Ray currentRay = ray;

    glm::vec3 accumulatedColor(0.0f);
    glm::vec3 energy(1.0f);

    for (int bounce = 0; bounce < maxBounces; ++bounce)
    {
        // 1. Trace the main ray
        auto hitOpt = TraceScene(currentRay, scene);
        if (!hitOpt.has_value())
        {
            // If there's an environment texture, sample it
            if (scene.GetEnvironmentTexture().has_value())
            {
                const auto& tex = scene.GetEnvironmentTexture().value();
                accumulatedColor +=
                    energy * glm::vec3(tex.SampleEquirectangular(currentRay.direction)) / 255.0f;
            }
            break;
        }

        // 2. Compute the bounce and energy loss
        const auto& hit = hitOpt.value();
        const auto& geomHit = RayHitGeometryInfo(hit);
        const auto& material = hit.meshInstance->GetMesh().GetMaterial();
        glm::vec3 direction = currentRay.direction;
        glm::vec3 energyMultiplier(1.0f);
        material->Reflect(geomHit, direction, energyMultiplier, rng);

        // 3. For each light:
        for (const auto& light : scene.GetLights())
        {
            // 3.1. Test shadow rays against the lights
            const auto lightPos = light.GetPosition();
            const auto newOrig = geomHit.worldPosition + geomHit.Normal * 0.001f;
            const auto lightRay = lightPos - newOrig;
            const auto lightDir = glm::normalize(lightRay);
            Ray shadowRay(newOrig, lightDir);
            bool inShadow = TraceScene(shadowRay, scene, true).has_value();

            // 3.2. Compute the ligths' contributions
            if (!inShadow)
            {
                accumulatedColor += material->ComputeLightContribution(
                    geomHit, currentRay.direction, lightRay, light.GetPointLight().Color);
            }
        }

        // 4. Update the current ray and energy
        currentRay = Ray(geomHit.worldPosition + geomHit.Normal * 0.001f, direction);
        energy *= energyMultiplier;
    }

    accumulatedColor += 0.25f;  // Simple ambient term
    accumulatedColor = glm::pow(accumulatedColor, glm::vec3(2.2f));
    return glm::u8vec4(glm::u8vec4(glm::clamp(accumulatedColor * 255.0f, 0.0f, 255.0f), 255));
}
