#include <mutex>

#include "glm/fwd.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <spdlog/spdlog.h>

#include <chrono>
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/intersect.hpp>
#include <memory>

#include "BRDF.hpp"
#include "CLI/Config.hpp"
#include "Intersect.hpp"
#include "Platform/Platform.hpp"
#include "Tracer.hpp"

std::random_device Tracer::Tracer::rd;
using JobData = Tracer::Tracer::JobData;

static const float pulloutEpsilon = 0.0001f;

static void BuildTBN(const glm::vec3& normal, glm::vec3& tangent, glm::vec3& bitangent) noexcept
{
    const auto up =
        (glm::abs(normal.z) < 0.999f) ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
    tangent = glm::normalize(glm::cross(up, normal));
    bitangent = glm::cross(normal, tangent);
}

static glm::vec3 SampleCosineHemisphere(JobData& jobData, const glm::vec3& normal) noexcept
{
    const float u1 = jobData.randomFloat(jobData.rng);
    const float u2 = jobData.randomFloat(jobData.rng);
    const float r = std::sqrt(u1);
    const float phi = 2.0f * glm::pi<float>() * u2;

    glm::vec3 tangent, bitangent;
    BuildTBN(normal, tangent, bitangent);

    const glm::vec3 localDir(
        r * std::cos(phi), r * std::sin(phi), std::sqrt(glm::max(0.0f, 1.0f - u1)));
    return glm::normalize(tangent * localDir.x + bitangent * localDir.y + normal * localDir.z);
}

static glm::vec3 SampleGGXDirection(
    JobData& jobData, const glm::vec3& incident, const glm::vec3& normal, float roughness) noexcept
{
    const float u1 = jobData.randomFloat(jobData.rng);
    const float u2 = jobData.randomFloat(jobData.rng);
    const float alpha = glm::max(roughness * roughness, 0.0025f);
    const float alpha2 = alpha * alpha;
    const float phi = 2.0f * glm::pi<float>() * u2;
    const float tanTheta2 = alpha2 * u1 / glm::max(1.0f - u1, 0.0001f);
    const float cosTheta = 1.0f / std::sqrt(1.0f + tanTheta2);
    const float sinTheta = std::sqrt(glm::max(0.0f, 1.0f - cosTheta * cosTheta));

    glm::vec3 tangent, bitangent;
    BuildTBN(normal, tangent, bitangent);

    glm::vec3 halfVector =
        glm::normalize(tangent * (sinTheta * std::cos(phi)) +
                       bitangent * (sinTheta * std::sin(phi)) + normal * cosTheta);
    if (glm::dot(halfVector, -incident) < 0.0f) halfVector = -halfVector;

    const glm::vec3 reflected = glm::reflect(incident, halfVector);
    return (glm::dot(reflected, normal) > 0.0f) ? glm::normalize(reflected)
                                                : SampleCosineHemisphere(jobData, normal);
}

static float ComputeSpecularProbability(const glm::vec3& viewDir, const glm::vec3& normal,
    const glm::vec3& baseColor, float metallic) noexcept
{
    const auto f0 = Tracer::BRDF::BaseReflectivity(baseColor, metallic);
    const auto fresnel =
        Tracer::BRDF::FresnelSchlick(glm::max(glm::dot(normal, viewDir), 0.0f), f0);
    const float probability = (fresnel.r + fresnel.g + fresnel.b) / 3.0f;
    return glm::clamp(probability, 0.05f, 0.95f);
}

[[maybe_unused]] static glm::vec3 GenerateRandomDirection(JobData& jobData) noexcept
{
    std::uniform_real_distribution<float> phi(0.0f, 2.0f * glm::pi<float>());
    std::uniform_real_distribution<float> theta(-glm::half_pi<float>(), glm::half_pi<float>());

    const float u = phi(jobData.rng);
    const float v = theta(jobData.rng);

    return glm::normalize(glm::vec3(cosf(u) * cosf(v), sinf(v), sinf(u) * cosf(v)));
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

static Tracer::Ray ReflectSpecular(
    JobData& jobData, const Tracer::RayHit& hit, const glm::vec3& normal, float roughness) noexcept
{
    const auto dir = SampleGGXDirection(jobData, hit.direction, normal, roughness);
    const auto pos = hit.origin + hit.direction * hit.distance + pulloutEpsilon * normal;
    return {pos, dir};
}

static Tracer::Ray ReflectDiffuse(
    JobData& jobData, const Tracer::RayHit& hit, const glm::vec3& normal) noexcept
{
    const auto dir = SampleCosineHemisphere(jobData, normal);
    const auto pos = hit.origin + hit.direction * hit.distance + pulloutEpsilon * normal;
    return {pos, dir};
}

void Tracer::Tracer::PrepareLights()
{
    simplifiedLights_.clear();
    for (const auto& light : scene_.GetLights())
    {
        const auto color = light.GetColor();
        const auto intensity = (color.r + color.g + color.b) / 3.0f;
        simplifiedLights_.push_back({light.GetPosition(), intensity});
    }
}

void Tracer::Tracer::GeneratePath(JobData& jobData, const Ray& ray, std::vector<PathStep>& pathVec,
    size_t maxBounces, float terminateEnergy) const noexcept
{
    Ray currentRay = ray, newRay;
    glm::vec3 totalEnergy(1.0f);
    pathVec.clear();

    while (pathVec.size() < maxBounces)
    {
        // Get the hit
        PathStep step;
        const auto hit = IntersectScene(currentRay, scene_);
        jobData.raysTraced++;
        if (!hit.has_value())
        {
            step.ray = currentRay;
            step.didHit = false;
            pathVec.push_back(step);
            break;
        }

        // Read and store the geometry info
        const auto geom = RayHitGeometryInfo(*hit);
        const auto& matRef = hit->meshInstance->GetMaterial();
        auto mat = matRef.SampleMaterial(geom.TexCoord0);
        mat.baseColor = mat.baseColor * 0.96f + 0.04f;
        step.ray = currentRay;
        step.hitPos = hit->worldPosition;
        step.baseColor = glm::vec3(mat.baseColor);
        step.roughness = mat.roughness;
        step.metallic = mat.metallic;
        step.normal = ComputeNormal(geom.Normal, mat.normal);
        step.emission = glm::vec3(mat.emission);
        step.didHit = true;

        // Compute the bounce direction and energy transfer
        const auto viewDir = -currentRay.direction;
        const auto specularProbability =
            ComputeSpecularProbability(viewDir, step.normal, step.baseColor, step.metallic);
        if (jobData.randomFloat(jobData.rng) < specularProbability)
        {
            newRay = ReflectSpecular(jobData, *hit, step.normal, mat.roughness);
        }
        else
        {
            newRay = ReflectDiffuse(jobData, *hit, step.normal);
        }

        const float diffusePdf = BRDF::DiffusePdf(newRay.direction, step.normal);
        const float specularPdf =
            BRDF::SpecularPdf(newRay.direction, viewDir, step.normal, step.roughness);
        const float pdf =
            (1.0f - specularProbability) * diffusePdf + specularProbability * specularPdf;
        const glm::vec3 brdf = BRDF::EvaluateBRDF(
            newRay.direction, viewDir, step.normal, step.baseColor, step.roughness, step.metallic);
        const float dotNL = glm::max(glm::dot(step.normal, newRay.direction), 0.0f);
        step.energy = (pdf > 0.0f) ? brdf * (dotNL / pdf) : glm::vec3(0.0f);

        // Store the new ray and energy transfer for the next bounce
        pathVec.push_back(step);
        currentRay = newRay;

        // Terminate early
        totalEnergy *= step.energy;
        if (glm::length(totalEnergy) < terminateEnergy) break;
    }
}

static glm::vec3 ApplyACESColorCorrection(const JobData& jobData, const glm::vec3& color) noexcept
{
    // Generated by ChatGPT
    auto col = color * jobData.exposureMultiplier;
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return (col * (a * col + b)) / (col * (c * col + d) + e);
}

static glm::vec3 ApplyColorCorrection(const JobData& jobData, const glm::vec3& color) noexcept
{
    return ApplyACESColorCorrection(jobData, color);
}

glm::vec3 Tracer::Tracer::ProcessRayForward(JobData& jobData, const Ray& ray) noexcept
{
    static thread_local std::vector<PathStep> path;
    const auto& cfg = Config::GetConfig();
    GeneratePath(jobData, ray, path, cfg.bounces, cfg.terminateEnergy);

    // Accumulate color from bounces
    glm::vec3 totalLight(0.0f), currentEnergy(1.0f);
    // NOLINTNEXTLINE(modernize-loop-convert)
    for (size_t index = 0; index < path.size(); ++index)
    {
        const auto& step = path[index];

        // If the path didn't hit, sample the skybox
        if (!step.didHit)
        {
            const glm::vec3 color = (index > 0) ? path[index - 1].baseColor : glm::vec3(1.0f);
            const bool prim = index == 0;
            totalLight += color * currentEnergy * scene_.SampleSkybox(step.ray.direction, prim);
            break;
        }

        // Add emission from the hit material
        totalLight += currentEnergy * cfg.emissionBaseIntensity * step.emission;

        int chosenLightIndex = -1;
        float chosenLightDistancePDF = 0.0f;
        float totalScore = 0.0f;

        // NOLINTNEXTLINE(modernize-loop-convert)
        // Find the light for this bounce with a running weighted random selection
        for (size_t i = 0; i < simplifiedLights_.size(); ++i)
        {
            const auto& light = simplifiedLights_[i];
            const auto lightPath = light.position - step.hitPos;
            const float distanceSquared = glm::dot(lightPath, lightPath);
            const float score = light.intensity / (distanceSquared + 1.0f);
            totalScore += score;
            if (jobData.randomFloat(jobData.rng) * totalScore < score)
            {
                chosenLightIndex = static_cast<int>(i);
                chosenLightDistancePDF = score;
            }
        }
        chosenLightDistancePDF /= totalScore;

        // Skip if no light was selected
        if (chosenLightIndex == -1) continue;

        // Sample the light with a shadow ray
        const auto& light = scene_.GetLights()[chosenLightIndex];

        const auto lightPos =
            light.GetPosition() + GenerateRandomDirection(jobData) * light.GetPointSize();
        const auto lightColor = light.GetColor();
        const glm::vec3 lightVec = lightPos - step.hitPos;
        const glm::vec3 lightDir = glm::normalize(lightVec);
        const Ray shadowRay(step.hitPos + lightVec * pulloutEpsilon, lightDir);
        const auto shadowHit = IntersectScene(shadowRay, scene_);
        jobData.raysTraced++;

        // If the shadow ray is not occluded, accumulate the light contribution
        if (!shadowHit.has_value() || shadowHit->distance > glm::length(lightVec))
        {
            float metalness = step.metallic;
            float roughness = step.roughness;
            const glm::vec3 brdf = BRDF::EvaluateBRDF(
                lightDir, -step.ray.direction, step.normal, step.baseColor, roughness, metalness);
            const float dotNL = glm::max(glm::dot(step.normal, lightDir), 0.0f);
            float distanceFalloff = 1.0f / (glm::length(lightVec) * glm::length(lightVec) + 1.0f);
            const glm::vec3 lightEnergy = brdf * dotNL * distanceFalloff;
            totalLight += currentEnergy * lightEnergy * lightColor / chosenLightDistancePDF;
        }

        // Update the current energy for the next bounce
        currentEnergy *= step.energy;
    }

    jobData.samplesTraced++;

    return ApplyColorCorrection(jobData, totalLight);
}

glm::vec3 Tracer::Tracer::ProcessRayBidirectional(JobData& jobData, const Ray& ray) noexcept
{
    return ProcessRayForward(jobData, ray);
}

static size_t CleanupFireflies(std::vector<glm::vec3>& buffer, float fireflyEliminationThreshold)
{
    auto colorToIntensity = [](const glm::vec3& color)
    { return (color.r + color.g + color.b) / 3.0f; };

    // Compute the average intensity
    auto imageSamples = buffer.size();
    const float averageIntensity =
        std::accumulate(buffer.begin(), buffer.end(), 0.0f,
            [&](float sum, const glm::vec3& color) { return sum + colorToIntensity(color); }) /
        static_cast<float>(imageSamples);

    // Compute standard deviation
    const float standardDeviation = std::accumulate(buffer.begin(), buffer.end(), 0.0f,
                                        [&](float sum, const glm::vec3& color)
                                        {
                                            const float diff =
                                                (colorToIntensity(color) - averageIntensity);
                                            return sum + diff * diff;
                                        }) /
                                    static_cast<float>(imageSamples);
    const float standardDeviationSqrt = std::sqrt(standardDeviation);

    // Zero out any samples that surpass the threshold, and compute the final color
    const float threshold = fireflyEliminationThreshold * standardDeviationSqrt;
    for (auto& color : buffer)
    {
        if (std::abs(colorToIntensity(color) - averageIntensity) > threshold)
        {
            color = glm::vec3(0.0f);
            imageSamples--;
        }
    }

    // Protect against divide-by-zero errors
    return (imageSamples == 0) ? 1 : imageSamples;
}

void Tracer::Tracer::RenderBlock(const Block& block)
{
    const auto& cfg = Config::GetConfig();
    const auto imageWidth = cfg.imageWidth;
    const auto imageHeight = cfg.imageHeight;
    const auto imageSamples = cfg.samples;
    const auto aspectRatio = static_cast<float>(imageWidth) / static_cast<float>(imageHeight);
    const auto pixelSize = glm::vec2(1.0f) / glm::vec2(imageWidth, imageHeight);
    const auto useStatisticFireflyElimination = cfg.useStatisticFireflyElimination;
    const auto fireflyEliminationThreshold = cfg.fireflyEliminationThreshold;
    const auto bidirectionalPathTracing = cfg.bidirectionalPathTracing;

    JobData jobData;
    jobData.rng.seed(rd());
    jobData.exposureMultiplier = std::powf(2.0f, -cfg.evExposure);

    std::uniform_real_distribution<float> xDist(0.0f, pixelSize.x);
    std::uniform_real_distribution<float> yDist(0.0f, pixelSize.y);

    auto blockBuffer = std::make_unique<RenderBuffer>(block.size.x, block.size.y);

    std::vector<glm::vec3> sampleBuffer(block.size.x * block.size.y, glm::vec3(0.0f));

    auto renderPixel = [&](int x, int y)
    {
        sampleBuffer.clear();
        const auto baseU = static_cast<float>(x) * pixelSize.x;
        const auto baseV = static_cast<float>(y) * pixelSize.y;

        // Collect the samples
        for (unsigned s = 0; s < imageSamples; ++s)
        {
            const auto u = baseU + xDist(jobData.rng);
            const auto v = baseV + yDist(jobData.rng);
            const Ray ray = cam_.GenerateCameraRay(u, v, aspectRatio);
            if (bidirectionalPathTracing)
                sampleBuffer.push_back(ProcessRayBidirectional(jobData, ray));
            else
                sampleBuffer.push_back(ProcessRayForward(jobData, ray));
        }

        // Optionally clean up fireflies before averaging
        auto actualImageSamples = imageSamples;
        if (useStatisticFireflyElimination)
            actualImageSamples = CleanupFireflies(sampleBuffer, fireflyEliminationThreshold);

        glm::vec3 color =
            std::accumulate(sampleBuffer.begin(), sampleBuffer.end(), glm::vec3(0.0f)) /
            static_cast<float>(actualImageSamples);
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

void Tracer::Tracer::WorkerThread()
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

Tracer::Tracer::Tracer(std::shared_ptr<RenderBuffer> imageBuffer, const Scene::Scene& scene)
    : imageBuffer_(std::move(imageBuffer)), scene_(scene), cam_(scene.GetCamera())
{
    const auto aspect = static_cast<float>(imageBuffer_->GetWidth()) /
                        static_cast<float>(imageBuffer_->GetHeight());
    cam_.CheckCameraAspectRatio(aspect);
    PrepareLights();
}

void Tracer::Tracer::StartRender()
{
    const auto& config = Config::GetConfig();
    const auto blockSize = config.imageBlockSize;
    const auto imageWidth = config.imageWidth;
    const auto imageHeight = config.imageHeight;
    const auto cpuAffinity = config.cpuAffinity;
    const auto numThreads = config.numThreads;

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
        workers_.emplace_back(&Tracer::Tracer::WorkerThread, this);
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

void Tracer::Tracer::WaitForRender()
{
    std::scoped_lock lock(workersMutex_);
    for (auto& thread : workers_)
    {
        try
        {
            thread.join();
        }
        catch (const std::system_error& e)
        {
            spdlog::error("Error joining worker thread: {}", e.what());
        }
    }
    workers_.clear();
}

void Tracer::Tracer::KillRender()
{
    renderKilled_ = true;
}

Tracer::Tracer::Stats Tracer::Tracer::GetStats() const
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
    stats.rays = raysTraced_.load();
    stats.samples = samplesTraced_.load();
    return stats;
}

bool Tracer::Tracer::IsDone() const
{
    return initialQueueSize_ &&  // A render was started
           blocks_.empty() &&    // No more blocks to render
           workers_.empty();     // All worker threads have finished
}
