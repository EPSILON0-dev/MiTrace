#include "Trace/Trace.hpp"

#include <spdlog/spdlog.h>

#include <cmath>
#include <glm/fwd.hpp>

#include "Config/Config.hpp"
#include "Trace/BRDF.hpp"
#include "Trace/RayHit.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/constants.hpp>
#include <glm/gtx/intersect.hpp>

static const float pulloutEpsilon = 0.0001f;

Trace::Trace(
    std::shared_ptr<RenderBuffer> imageBuffer, const Camera& camera, const Scene& scene) noexcept
    : imageBuffer_(imageBuffer), camera_(camera), scene_(scene), rd_(), rng_(rd_())
{
}

std::optional<RayHit> Trace::TraceScene(const Ray& ray, const Scene& scene, bool anyHit) noexcept
{
    float lowestDistance = std::numeric_limits<float>::max();
    std::optional<RayHit> bestHit = std::nullopt;

    for (const auto& meshInstance : scene.GetMeshInstances())
    {
        if (const auto hit = meshInstance.IntersectRay(ray); hit.has_value())
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
    constexpr int maxBounces = 4;
    std::uniform_real_distribution<float> randomFloat(0.0f, 1.0f);

    Bounce bounces[maxBounces];
    int bounceCount = 0;
    Ray newRay, currentRay = ray;
    glm::vec3 totalEnergy(1.0f);

    for (bounceCount = 0; bounceCount < maxBounces; ++bounceCount)
    {
        const auto hit = TraceScene(currentRay, scene_);
        if (!hit.has_value()) break;

        const auto geom = RayHitGeometryInfo(*hit);
        const auto& matRef = hit->meshInstance->GetMesh().GetMaterial();
        const auto mat = matRef->SampleMaterial(geom.TexCoord0);

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
            const auto pointLight = light.GetPointLight();
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
            float lightDivisor = 500.0f;
            float brdf = BRDF::BRDF(glm::normalize(lightDir), -bounce.incomingRay.direction,
                bounce.effectiveNormal, roughness, metalness);
            float distanceFalloff = 1.0f / (glm::length(lightDir) * glm::length(lightDir));
            float lightEnergy = brdf * distanceFalloff / lightDivisor;

            if (!shadowHit.has_value() || shadowHit->distance > glm::length(lightDir))
            {
                totalLight += currentEnergy * lightEnergy *
                              glm::vec3(bounce.materialPoint.baseColor) * pointLight.Color *
                              pointLight.Intensity;
            }
        }

        currentEnergy *= bounce.energyTransfer;
    }

    // Hopefully will reduce fireflies
    return glm::clamp(totalLight, glm::vec3(0.0f), glm::vec3(5.0f));
}

void Trace::RenderNormal()
{
    const auto& config = Config::Instance().GetConfig();
    const auto imageWidth = config.image.width;
    const auto imageHeight = config.image.height;
    const auto imageSamples = config.image.samples;
    const auto aspectRatio = static_cast<float>(imageWidth) / imageHeight;
    const auto pixelSize = glm::vec2(1.0f) / glm::vec2(imageWidth, imageHeight);

    std::uniform_real_distribution<float> xDist(0.0f, pixelSize.x);
    std::uniform_real_distribution<float> yDist(0.0f, pixelSize.y);

    for (unsigned y = 0; y < imageHeight; ++y)
    {
        for (unsigned x = 0; x < imageWidth; ++x)
        {
            const auto baseU = (static_cast<float>(x) + 0.5f) * pixelSize.x;
            const auto baseV = (static_cast<float>(y) + 0.5f) * pixelSize.y;
            glm::vec3 color(0.0f);

            for (unsigned s = 0; s < imageSamples; ++s)
            {
                const auto u = baseU + xDist(rng_);
                const auto v = baseV + yDist(rng_);
                const Ray ray = camera_.GenerateRay(u, v, aspectRatio);
                color += ProcessRay(ray);
            }
            color /= static_cast<float>(imageSamples);
            imageBuffer_->SetPixel(x, y, color);
        }
    }
}

/*
std::vector<Trace::BucketJob> Trace::GenerateJobs() noexcept
{
    const auto& config = Config::Instance().GetConfig();

    const auto bucketSize = config.render.pixelsPerBucket;
    const auto bucketSamples = config.render.samplesPerBucket;
    const auto imageWidth = config.image.width;
    const auto imageHeight = config.image.height;
    const auto imageSamples = config.image.samples;

    const auto bucketsX = (imageWidth + bucketSize - 1) / bucketSize;
    const auto bucketsY = (imageHeight + bucketSize - 1) / bucketSize;
    const auto bucketsZ = (imageSamples + bucketSamples - 1) / bucketSamples;
    const auto totalJobs = bucketsX * bucketsY * bucketsZ;

    std::vector<BucketJob> jobs;
    jobs.reserve(totalJobs);

    // For spacial coherency we want to group Z buckets together
    for (unsigned by = 0; by < bucketsY; ++by)
    {
        for (unsigned bx = 0; bx < bucketsX; ++bx)
        {
            const glm::ivec2 offset{
                static_cast<int>(bx * bucketSize),
                static_cast<int>(by * bucketSize),
            };
            const glm::ivec2 size{
                std::min(bucketSize, imageWidth - offset.x),
                std::min(bucketSize, imageHeight - offset.y),
            };

            for (unsigned bz = 0; bz < bucketsZ; ++bz)
            {
                const int samples =
                    std::min(bucketSamples, imageSamples - static_cast<int>(bz * bucketSamples));
                jobs.push_back(BucketJob{offset, size, samples});
            }
        }
    }

    return jobs;
}

void Trace::GenerateBucket(
    BucketRays& bucket, glm::ivec2 offset, glm::ivec2 size, int samples) noexcept
{
    const auto imageWidth = imageBuffer_->GetWidth();
    const auto imageHeight = imageBuffer_->GetHeight();
    const auto pixelSize = glm::vec2(1.0f) / glm::vec2(imageWidth, imageHeight);
    const auto aspectRatio = static_cast<float>(imageWidth) / imageHeight;

    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_real_distribution<float> xDist(0.0f, pixelSize.x);
    std::uniform_real_distribution<float> yDist(0.0f, pixelSize.y);

    bucket.clear();
    for (int y = 0; y < size.y; ++y)
    {
        for (int x = 0; x < size.x; ++x)
        {
            const auto baseUV = (glm::vec2(offset) + glm::vec2(x, y)) * pixelSize;
            for (int s = 0; s < samples; ++s)
            {
                const auto uv = baseUV + glm::vec2(xDist(rng), yDist(rng));
                const Ray ray = camera_.GenerateRay(uv.x, uv.y, aspectRatio);
                bucket.push_back(ray);
            }
        }
    }
}

void Trace::ProcessBucket(const BucketRays& rays, BucketResult& result) noexcept
{
    for (const auto& ray : rays) result.push_back(ProcessRay(ray));
}

void Trace::WriteBucketToImage(const BucketJob& job, const BucketResult& result) noexcept
{
    const auto samples = job.samples;

    int colorIndex = 0;
    for (int y = 0; y < job.size.y; ++y)
    {
        for (int x = 0; x < job.size.x; ++x)
        {
            glm::vec3 accumulatedColor{0.0f, 0.0f, 0.0f};
            for (int s = 0; s < samples; ++s) accumulatedColor += result[colorIndex++];
            const glm::vec3 finalColor = accumulatedColor / static_cast<float>(samples);
            imageBuffer_->AddColorAt(job.offset.x + x, job.offset.y + y, finalColor);
        }
    }
}

void Trace::NormalizeImage() noexcept
{
    const auto& config = Config::Instance().GetConfig();
    const auto totalSamples = static_cast<float>(config.image.samples);

    for (unsigned y = 0; y < imageBuffer_->GetHeight(); ++y)
    {
        for (unsigned x = 0; x < imageBuffer_->GetWidth(); ++x)
        {
            glm::vec3 color;
            imageBuffer_->GetPixel(x, y, color);
            color /= totalSamples;
            imageBuffer_->SetPixel(x, y, color);
        }
    }
}


void Trace::RenderBucketted()
{
    const auto& bucketConf = Config::Instance().GetConfig().render;
    const auto pixels = bucketConf.pixelsPerBucket;
    const auto samples = bucketConf.samplesPerBucket;
    const auto totalRays = pixels * pixels * samples;

    BucketRays bucketRays;
    bucketRays.reserve(totalRays);
    spdlog::debug("Created a bucket for {} rays", totalRays);

    std::vector<BucketJob> bucketJobs = GenerateJobs();
    std::vector<BucketResult> bucketResults;
    spdlog::debug("Generated {} bucket jobs", bucketJobs.size());

    bucketResults.reserve(bucketJobs.size());
    for (size_t i = 0; i < bucketJobs.size(); ++i)
    {
        bucketResults.emplace_back();
        bucketResults.back().reserve(totalRays);
    }

    BucketResult bucketResult;
    bucketResult.reserve(totalRays);

    for (size_t i = 0; i < bucketJobs.size(); ++i)
    {
        const auto& job = bucketJobs[i];
        bucketResults[i].clear();
        GenerateBucket(bucketRays, job.offset, job.size, job.samples);
        ProcessBucket(bucketRays, bucketResults[i]);
    }

    for (size_t i = 0; i < bucketJobs.size(); ++i)
        WriteBucketToImage(bucketJobs[i], bucketResults[i]);

    NormalizeImage();
}
*/
