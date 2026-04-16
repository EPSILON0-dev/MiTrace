#define GLM_ENABLE_EXPERIMENTAL
#include "Tracer.hpp"

#include <spdlog/spdlog.h>

#include <chrono>
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/intersect.hpp>
#include <memory>

#include "BRDF.hpp"
#include "CLI/Config.hpp"
#include "Intersect.hpp"
#include "Platform/Platform.hpp"

using namespace BasicBackend;
static const float pulloutEpsilon = 0.0001f;

std::random_device BasicTracer::rd;

BasicTracer::BasicTracer(std::shared_ptr<RenderBuffer> imageBuffer, const Scene::Scene& scene)
    : imageBuffer_(std::move(imageBuffer)), scene_(scene), cam_(scene.GetCamera())
{
    const auto aspect = static_cast<float>(imageBuffer_->GetWidth()) /
                        static_cast<float>(imageBuffer_->GetHeight());
    cam_.CheckCameraAspectRatio(aspect);
}

glm::vec3 BasicTracer::GenerateRandomDirection(JobPersistentData& jobData) noexcept
{
    std::uniform_real_distribution<float> phi(0.0f, 2.0f * glm::pi<float>());
    std::uniform_real_distribution<float> theta(-glm::half_pi<float>(), glm::half_pi<float>());

    const float u = phi(jobData.rng);
    const float v = theta(jobData.rng);

    return glm::normalize(glm::vec3(cosf(u) * cosf(v), sinf(v), sinf(u) * cosf(v)));
}

glm::vec3 BasicTracer::GenerateHemisphereDirection(
    JobPersistentData& jobData, const glm::vec3& normal) noexcept
{
    const auto dir = GenerateRandomDirection(jobData);
    return (glm::dot(dir, normal) > 0.0f) ? dir : -dir;
}

static glm::vec3 ComputeNormal(const glm::vec3& surfNorm, const glm::vec3& texNorm) noexcept
{
    const auto normal = glm::normalize(surfNorm);
    const auto up =
        (glm::abs(normal.y) < 0.9f) ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
    const auto tangent = glm::normalize(glm::cross(normal, up));
    const auto bitangent = glm::normalize(glm::cross(tangent, normal));

    const auto tbn = glm::mat3(tangent, bitangent, normal);
    return glm::normalize(tbn * texNorm);
}

Ray BasicTracer::ReflectSpecular(JobPersistentData& jobData, const RayHit& hit,
    const glm::vec3& normal, float roughness) noexcept
{
    const auto dirDiffuse = glm::normalize(GenerateHemisphereDirection(jobData, normal));
    const auto dirSpecular = glm::normalize(glm::reflect(hit.direction, normal));
    const auto dir = glm::normalize(glm::mix(dirSpecular, dirDiffuse, roughness));
    const auto pos = hit.origin + hit.direction * hit.distance + pulloutEpsilon * normal;
    return {pos, dir};
}

Ray BasicTracer::ReflectDiffuse(
    JobPersistentData& jobData, const RayHit& hit, const glm::vec3& normal) noexcept
{
    const auto dir = glm::normalize(GenerateHemisphereDirection(jobData, normal));
    const auto pos = hit.origin + hit.direction * hit.distance + pulloutEpsilon * normal;
    return {pos, dir};
}

void BasicTracer::GeneratePath(JobPersistentData& jobData, const Ray& ray,
    std::vector<PathStep>& pathVec, size_t maxBounces, float terminateEnergy) const noexcept
{
    std::uniform_real_distribution<float> randomFloat(0.0f, 1.0f);
    Ray currentRay = ray, newRay;
    glm::vec3 totalEnergy(1.0f);
    pathVec.clear();

    while (pathVec.size() < maxBounces)
    {
        // Get the hit
        PathStep step;
        const auto hit = BasicBackend::IntersectScene(currentRay, scene_);
        jobData.raysTraced++;
        if (!hit.has_value()) break;

        // Read and store the geometry info
        const auto geom = RayHitGeometryInfo(*hit);
        const auto& matRef = hit->meshInstance->GetMaterial();
        auto mat = matRef.SampleMaterial(geom.TexCoord0);
        mat.baseColor = mat.baseColor * 0.96f + 0.04f;
        step.ray = ray;
        step.hitPos = hit->worldPosition;
        step.baseColor = mat.baseColor;
        step.roughness = mat.roughness;
        step.metallic = mat.metallic;
        step.normal = ComputeNormal(geom.Normal, mat.normal);

        // Compute the bounce direction and energy transfer
        const auto fresnel = BasicBackend::BRDF::FresnelSchlick(
            glm::max(glm::dot(-currentRay.direction, step.normal), 0.0f), mat.metallic);
        if (randomFloat(jobData.rng) > fresnel)
        {
            newRay = ReflectDiffuse(jobData, *hit, step.normal);
        }
        else
        {
            newRay = ReflectSpecular(jobData, *hit, step.normal, mat.roughness);
        }
        float brdf = BasicBackend::BRDF::BRDF(
            newRay.direction, -currentRay.direction, step.normal, mat.roughness, mat.metallic);
        step.energy = glm::min(brdf * step.baseColor, glm::vec3(1.0f));
        // TODO Divide step energy by a PDF

        // Store the new ray and energy transfer for the next bounce
        pathVec.push_back(step);
        currentRay = newRay;

        // Terminate early
        totalEnergy *= step.energy;
        if (glm::length(totalEnergy) < terminateEnergy) break;
    }
}

glm::vec3 BasicTracer::ProcessRay(JobPersistentData& jobData, const Ray& ray) noexcept
{
    static thread_local std::vector<PathStep> path;
    const auto& config = Config::GetConfig();
    GeneratePath(jobData, ray, path, config.rendering.maxBounces, config.rendering.terminateEnergy);

    // Accumulate color from bounces
    glm::vec3 totalLight(0.0f), currentEnergy(1.0f);
    for (const auto& step : path)
    {
        // TODO Use better light selection than "we greedy, check all"
        // TODO Possibly add an optimization structure that preoptimizes light reach area
        for (const auto& light : scene_.GetLights())
        {
            // For now we assume that all lights are point lights
            const auto lightPos =
                light.GetPosition() + GenerateRandomDirection(jobData) * light.GetPointSize();
            const auto lightColor = light.GetColor();
            const glm::vec3 lightDir = lightPos - step.hitPos;
            const Ray shadowRay(step.hitPos + lightDir * pulloutEpsilon, lightDir);
            const auto shadowHit = BasicBackend::IntersectScene(shadowRay, scene_);
            jobData.raysTraced++;

            // TODO combine into one BRDF function
            float metalness = step.metallic;
            float roughness = step.roughness;
            float lightDivisor = 1000.0f;
            float brdf = BasicBackend::BRDF::BRDF(
                glm::normalize(lightDir), -step.ray.direction, step.normal, roughness, metalness);
            float distanceFalloff = 1.0f / (glm::length(lightDir) * glm::length(lightDir) + 1.0f);
            float lightEnergy = brdf * distanceFalloff / lightDivisor;

            if (!shadowHit.has_value() || shadowHit->distance > glm::length(lightDir))
            {
                totalLight += currentEnergy * lightEnergy * glm::vec3(step.baseColor) * lightColor;
            }
        }

        currentEnergy *= step.energy;
    }

    jobData.samplesTraced++;
    return totalLight;
    // Hopefully will reduce fireflies
    // return glm::clamp(totalLight, glm::vec3(0.0f), glm::vec3(5.0f));
}

void BasicTracer::RenderBlock(const Block& block)
{
    const auto& config = Config::GetConfig();
    const auto imageWidth = config.image.width;
    const auto imageHeight = config.image.height;
    const auto imageSamples = config.image.samples;
    const auto aspectRatio = static_cast<float>(imageWidth) / static_cast<float>(imageHeight);
    const auto pixelSize = glm::vec2(1.0f) / glm::vec2(imageWidth, imageHeight);

    JobPersistentData jobData;
    jobData.rng.seed(rd());

    std::uniform_real_distribution<float> xDist(0.0f, pixelSize.x);
    std::uniform_real_distribution<float> yDist(0.0f, pixelSize.y);

    auto blockBuffer = std::make_unique<RenderBuffer>(block.size.x, block.size.y);
    auto renderPixel = [&](int x, int y)
    {
        const auto baseU = static_cast<float>(x) * pixelSize.x;
        const auto baseV = static_cast<float>(y) * pixelSize.y;
        glm::vec3 color(0.0f);

        for (unsigned s = 0; s < imageSamples; ++s)
        {
            const auto u = baseU + xDist(jobData.rng);
            const auto v = baseV + yDist(jobData.rng);
            const Ray ray = cam_.GenerateCameraRay(u, v, aspectRatio);
            color += ProcessRay(jobData, ray);
        }
        color /= static_cast<float>(imageSamples);
        blockBuffer->SetPixel(x - block.offset.x, y - block.offset.y, color);
    };

    for (int y = block.offset.y; y < block.offset.y + block.size.y; ++y)
    {
        for (int x = block.offset.x; x < block.offset.x + block.size.x; ++x)
        {
            renderPixel(x, y);
        }
    }

    imageBuffer_->DrawSubBuffer(*blockBuffer, block.offset.x, block.offset.y);

    samplesTraced_ += jobData.samplesTraced;
    raysTraced_ += jobData.raysTraced;
}

void BasicTracer::WorkerThread()
{
    while (!renderKilled_)
    {
        Block block;
        {
            std::scoped_lock<std::mutex> lock(blockMutex_);
            if (blocks_.empty()) break;
            block = blocks_.front();
            blocks_.pop();
        }
        RenderBlock(block);
    }
}

void BasicTracer::StartRender()
{
    const auto& config = Config::GetConfig();
    const auto blockSize = config.rendering.blockSize;
    const auto imageWidth = config.image.width;
    const auto imageHeight = config.image.height;
    const auto cpuAffinity = config.rendering.cpuAffinity;
    const auto numThreads = config.rendering.numThreads;

    // Prepare blocks for rendering
    for (unsigned y = 0; y < imageHeight; y += blockSize)
    {
        for (unsigned x = 0; x < imageWidth; x += blockSize)
        {
            auto offset = glm::ivec2(x, y);
            auto sizeX = std::min(blockSize, imageWidth - x);
            auto sizeY = std::min(blockSize, imageHeight - y);
            blocks_.push(Block{offset, glm::ivec2(sizeX, sizeY)});
        }
    }
    initialQueueSize_ = blocks_.size();
    spdlog::info("Generated {} jobs of size {}x{}", blocks_.size(), blockSize, blockSize);

    // Warn about thread count
    if (numThreads > std::thread::hardware_concurrency())
    {
        spdlog::warn(
            "Configured thread count ({}) exceeds hardware concurrency ({}). This may lead to "
            "performance degradation.",
            numThreads, std::thread::hardware_concurrency());
    }

    // Start and join worker threads
    startTime_ = std::chrono::system_clock::now();
    for (unsigned i = 0; i < numThreads; ++i)
        workers_.emplace_back(&BasicTracer::WorkerThread, this);
    spdlog::info("Started {} worker threads", numThreads);

    // Set CPU affinity if enabled
    if (cpuAffinity)
    {
        if (numThreads > std::thread::hardware_concurrency())
        {
            spdlog::warn(
                "Unable to set CPU affinity for all worker threads due to thread count exceeding "
                "hardware concurrency.");
        }
        else
        {
            // This is calculated to spread threads across cores as evenly as possible,
            // if there are 2 threads and 4 cores, we want them to use core 0 and 2, which on
            // hyperthreaded CPUs will often be on different physical cores/
            auto numCores = std::thread::hardware_concurrency();
            auto coreOffset = numCores / numThreads;
            for (unsigned i = 0; i < numThreads; ++i)
            {
                Platform::SetThreadAffinity(workers_[i], i * coreOffset);
            }
            spdlog::info("Set CPU affinity for worker threads");
        }
    }
}

void BasicTracer::WaitForRender()
{
    for (auto& thread : workers_) thread.join();
    workers_.clear();
}

void BasicTracer::KillRender()
{
    renderKilled_ = true;
    WaitForRender();
}

Tracer::Stats BasicTracer::GetStats() const
{
    using std::chrono::duration_cast;
    using std::chrono::seconds;
    using std::chrono::system_clock;

    Stats stats;
    stats.progress =
        1.0f - static_cast<float>(blocks_.size()) / static_cast<float>(initialQueueSize_);
    stats.timeElapsed =
        static_cast<float>(duration_cast<seconds>(system_clock::now() - startTime_).count());
    stats.estimatedTimeRemaining = (stats.timeElapsed / stats.progress) * (1.0f - stats.progress);
    stats.raysTraced = raysTraced_.load();
    stats.samplesTraced = samplesTraced_.load();
    return stats;
}

bool BasicTracer::IsDone() const
{
    return initialQueueSize_ &&  // A render was started
           blocks_.empty() &&    // No more blocks to render
           workers_.empty();     // All worker threads have finished
}
