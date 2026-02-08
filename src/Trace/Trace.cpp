#include "Trace/Trace.hpp"

#include <spdlog/spdlog.h>

#include <cmath>
#include <glm/fwd.hpp>

#include "Loader/Config.hpp"
#include "Scene/Mesh.hpp"
#include "Trace/BRDF.hpp"
#include "Trace/Intersect.hpp"
#include "Trace/RayHit.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/constants.hpp>
#include <glm/gtx/intersect.hpp>

Ray Trace::GenerateCameraRay(float u, float v, float aspectRatio) const noexcept
{
    float fovScale = tanf(camera_.GetFov() * 0.5f);
    float px = (2.0f * u - 1.0f) * fovScale;
    float py = (2.0f * v - 1.0f) * fovScale / aspectRatio;

    glm::vec4 rayOriginCameraSpace = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    glm::vec4 rayDirectionCameraSpace = glm::normalize(glm::vec4(px, -py, -1.0f, 0.0f));

    glm::vec4 rayOriginWorldSpace = camera_.GetCameraToWorld() * rayOriginCameraSpace;
    glm::vec4 rayDirectionWorldSpace = camera_.GetCameraToWorld() * rayDirectionCameraSpace;

    return Ray(glm::vec3(rayOriginWorldSpace), glm::normalize(glm::vec3(rayDirectionWorldSpace)));
}

static const float pulloutEpsilon = 0.0001f;

Trace::Trace(std::shared_ptr<RenderBuffer> imageBuffer, const Scene::Camera& camera,
    const Scene::Scene& scene) noexcept
    : imageBuffer_(imageBuffer), camera_(camera), scene_(scene), rd_(), rng_(rd_())
{
}

std::optional<RayHit> Trace::TraceScene(
    const Ray& ray, const Scene::Scene& scene, bool anyHit) noexcept
{
    float lowestDistance = std::numeric_limits<float>::max();
    std::optional<RayHit> bestHit = std::nullopt;

    for (const auto& meshInstance : scene.GetMeshInstances())
    {
        if (const auto hit = IntersectMeshInstance(ray, meshInstance); hit.has_value())
        {
            if (anyHit) break;
            if (hit->distance < lowestDistance)
            {
                lowestDistance = hit->distance;
                bestHit = *hit;
            }
        }
    }

    return bestHit;
}

glm::vec3 Trace::GenerateHemisphereDirection(const glm::vec3& normal) noexcept
{
    std::uniform_real_distribution<float> phi(0.0f, 2.0f * glm::pi<float>());
    std::uniform_real_distribution<float> theta(-glm::half_pi<float>(), glm::half_pi<float>());

    const float u = phi(rng_);
    const float v = theta(rng_);

    const glm::vec3 dir = glm::normalize(glm::vec3(cosf(u) * cosf(v), sinf(v), sinf(u) * cosf(v)));
    return (glm::dot(dir, normal) > 0.0f) ? dir : -dir;
}

glm::vec3 Trace::ComputeNormal(const glm::vec3& surfNorm, const glm::vec3& texNorm) noexcept
{
    // FIXME currently broken
    glm::vec3 tangent, bitangent;
    glm::vec3 n = glm::normalize(surfNorm);
    if (std::abs(n.x) > std::abs(n.z))
        tangent = glm::normalize(glm::vec3(-n.y, n.x, 0.0f));
    else
        tangent = glm::normalize(glm::vec3(0.0f, -n.z, n.y));
    bitangent = glm::cross(n, tangent);

    glm::mat3 tbn = glm::mat3(tangent, bitangent, n);
    glm::vec3 mappedNormal = texNorm * 2.0f - glm::vec3(1.0f);
    return glm::normalize(tbn * mappedNormal);
}

Ray Trace::ReflectSpecular(const RayHit& hit, const glm::vec3& normal, float roughness) noexcept
{
    const auto dirDiffuse = glm::normalize(GenerateHemisphereDirection(normal));
    const auto dirSpecular = glm::normalize(glm::reflect(hit.direction, normal));
    const auto dir = glm::normalize(glm::mix(dirSpecular, dirDiffuse, roughness));
    const auto pos = hit.origin + hit.direction * hit.distance + pulloutEpsilon * normal;
    return Ray(pos, dir);
}

Ray Trace::ReflectDiffuse(const RayHit& hit, const glm::vec3& normal) noexcept
{
    const auto dir = glm::normalize(GenerateHemisphereDirection(normal));
    const auto pos = hit.origin + hit.direction * hit.distance + pulloutEpsilon * normal;
    return Ray(pos, dir);
}

glm::vec3 Trace::ProcessRay(const Ray& ray) noexcept
{
    constexpr int bounceArraySize = 16;
    int maxBounces = Config::GetConfig().rendering.maxBounces;

    std::uniform_real_distribution<float> randomFloat(0.0f, 1.0f);

    Bounce bounces[bounceArraySize];
    int bounceCount = 0;
    Ray newRay, currentRay = ray;
    glm::vec3 totalEnergy(1.0f);

    for (bounceCount = 0; bounceCount < maxBounces; ++bounceCount)
    {
        const auto hit = TraceScene(currentRay, scene_);
        if (!hit.has_value())
        {
            ++bounceCount;
            break;
        }

        const auto geom = RayHitGeometryInfo(*hit);
        const auto& matRef = hit->meshInstance->GetMaterial();
        const auto mat = matRef.SampleMaterial(geom.TexCoord0);

        bounces[bounceCount].incomingRay = ray;
        bounces[bounceCount].hitInfo = *hit;
        bounces[bounceCount].materialPoint = mat;

        const auto fresnel = BRDF::FresnelSchlick(
            glm::max(glm::dot(-currentRay.direction, geom.Normal), 0.0f), mat.metallic);

        const auto normal = geom.Normal;  // TODO ComputeNormal(geom.Normal, mat.normal);
        float energyTransfer = 1.0f;
        if (randomFloat(rng_) > fresnel)
        {
            newRay = ReflectDiffuse(*hit, normal);
            energyTransfer = BRDF::BRDF(
                newRay.direction, -currentRay.direction, normal, mat.roughness, mat.metallic);
        }
        else
        {
            newRay = ReflectSpecular(*hit, normal, mat.roughness);
        }
        // TODO Divide the sample by the PDF

        bounces[bounceCount].effectiveNormal = normal;
        bounces[bounceCount].energyTransfer = glm::vec3(mat.baseColor) * energyTransfer;
        bounces[bounceCount].outgoingRay = newRay;
        currentRay = newRay;

        // Terminate early
        totalEnergy *= bounces[bounceCount].energyTransfer;
        if (glm::length(totalEnergy) < 0.01f)
        {
            ++bounceCount;
            break;
        }
    }

    // Accumulate color from bounces
    glm::vec3 totalLight(0.0f), currentEnergy(1.0f);
    for (int i = 0; i < bounceCount; i++)
    {
        const auto& bounce = bounces[i];

        for (const auto& light : scene_.GetLights())
        {
            // For now we assume that all lights are point lights
            const auto lightColor = light.GetColor();
            const glm::vec3 lightDir = light.GetPosition() - bounce.hitInfo.worldPosition;
            const Ray shadowRay(bounce.hitInfo.worldPosition + lightDir * pulloutEpsilon, lightDir);
            const auto shadowHit = TraceScene(shadowRay, scene_);

            /*
            float frenelFactor = bounce.materialPoint.metallic;
            float lightDivisor = 500.0f;
            float lightDot =
                glm::max(glm::dot(bounce.effectiveNormal, glm::normalize(lightDir)), 0.0f);
            float lightDiffuse = lightDot * (1.0f - frenelFactor);
            float lightSpecular =
                std::powf(lightDot, 250.0f * (1.0f - bounce.materialPoint.roughness) + 10.0f) *
                frenelFactor;
            float distanceFalloff = 1.0f / (glm::length(lightDir) * glm::length(lightDir));
            float lightEnergy = (lightDiffuse * distanceFalloff + lightSpecular) / lightDivisor;
            */

            float metalness = bounce.materialPoint.metallic;
            float roughness = bounce.materialPoint.roughness;
            float lightDivisor = 350.0f;
            float brdf = BRDF::BRDF(glm::normalize(lightDir), -bounce.incomingRay.direction,
                bounce.effectiveNormal, roughness, metalness);
            float distanceFalloff = 1.0f / (glm::length(lightDir) * glm::length(lightDir) + 1.0f);
            float lightEnergy = brdf * distanceFalloff / lightDivisor;

            if (!shadowHit.has_value() || shadowHit->distance > glm::length(lightDir))
            {
                totalLight += currentEnergy * lightEnergy *
                              glm::vec3(bounce.materialPoint.baseColor) * lightColor;
            }
        }

        currentEnergy *= bounce.energyTransfer;
    }

    // Hopefully will reduce fireflies
    return glm::clamp(totalLight, glm::vec3(0.0f), glm::vec3(5.0f));
}

void Trace::RenderNormal()
{
    const auto& config = Config::GetConfig();
    const auto blockSize = config.rendering.blockSize;
    const auto imageWidth = config.image.width;
    const auto imageHeight = config.image.height;
    const auto imageSamples = config.image.samples;
    const auto aspectRatio = static_cast<float>(imageWidth) / imageHeight;
    const auto pixelSize = glm::vec2(1.0f) / glm::vec2(imageWidth, imageHeight);

    std::uniform_real_distribution<float> xDist(0.0f, pixelSize.x);
    std::uniform_real_distribution<float> yDist(0.0f, pixelSize.y);

    auto renderPixel = [&](int x, int y)
    {
        const auto baseU = (static_cast<float>(x) + 0.5f) * pixelSize.x;
        const auto baseV = (static_cast<float>(y) + 0.5f) * pixelSize.y;
        glm::vec3 color(0.0f);

        for (unsigned s = 0; s < imageSamples; ++s)
        {
            const auto u = baseU + xDist(rng_);
            const auto v = baseV + yDist(rng_);
            const Ray ray = GenerateCameraRay(u, v, aspectRatio);
            color += ProcessRay(ray);
        }
        color /= static_cast<float>(imageSamples);
        imageBuffer_->SetPixel(x, y, color);
    };

    for (unsigned by = 0; by < imageHeight; by += blockSize)
    {
        for (unsigned bx = 0; bx < imageWidth; bx += blockSize)
        {
            for (unsigned y = 0; y < blockSize && by + y < imageHeight; ++y)
            {
                for (unsigned x = 0; x < blockSize && bx + x < imageWidth; ++x)
                {
                    renderPixel(bx + x, by + y);
                }
            }
        }
    }
}
