#include "Trace/Trace.hpp"

#include <cstddef>
#include <random>
#include <spdlog/spdlog.h>

#include "Config/Config.hpp"
#include "Trace/RayHit.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/intersect.hpp>

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

glm::vec3 Trace::ProcessRay(const Ray& ray) noexcept
{
    const auto hit = TraceScene(ray, scene_);
    if (hit.has_value())
    {
        const auto& mat = hit->meshInstance->GetMesh().GetMaterial().get();
        const auto geom = RayHitGeometryInfo(*hit);
        const auto uv = geom.TexCoord0;
        return mat->GetBaseColor(uv);
    }
    else
    {
        return glm::vec3{0.0f, 0.0f, 0.0f};
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

void Trace::RenderNormal()
{
    const auto& config = Config::Instance().GetConfig();
    const auto imageWidth = config.image.width;
    const auto imageHeight = config.image.height;
    const auto imageSamples = config.image.samples;
    const auto aspectRatio = static_cast<float>(imageWidth) / imageHeight;
    const auto pixelSize = glm::vec2(1.0f) / glm::vec2(imageWidth, imageHeight);

    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_real_distribution<float> xDist(0.0f, pixelSize.x);
    std::uniform_real_distribution<float> yDist(0.0f, pixelSize.y);

    {
        for (unsigned y = 0; y < imageHeight; ++y)
        {
            for (unsigned x = 0; x < imageWidth; ++x)
            {
                const auto baseU = (static_cast<float>(x) + 0.5f) * pixelSize.x;
                const auto baseV = (static_cast<float>(y) + 0.5f) * pixelSize.y;
                glm::vec3 color(0.0f);

                for (unsigned s = 0; s < imageSamples; ++s)
                {
                    const auto u = baseU + xDist(rng);
                    const auto v = baseV + yDist(rng);
                    const Ray ray = camera_.GenerateRay(u, v, aspectRatio);
                    color += ProcessRay(ray);
                }
                imageBuffer_->SetPixel(x, y, color);
            }
        }
    }

    NormalizeImage();
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
