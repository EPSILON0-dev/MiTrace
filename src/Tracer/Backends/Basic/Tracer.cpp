#include "glm/fwd.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <spdlog/spdlog.h>

#include <chrono>
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/intersect.hpp>

#include "BRDF.hpp"
#include "CLI/Config.hpp"
#include "Intersect.hpp"
#include "Platform/Platform.hpp"
#include "Scene/Mesh.hpp"
#include "Tracer.hpp"

using namespace BasicBackend;
static const float pulloutEpsilon = 0.0001f;

thread_local std::random_device BasicTracer::rd;
thread_local std::mt19937 BasicTracer::rng;

void BasicTracer::CheckCameraAspectRatio(float renderAspectRatio) const noexcept
{
    if (fabsf(scene_.GetCamera().GetAspectRatio() - renderAspectRatio) < 0.01f) return;

    spdlog::warn(
        "Camera aspect ratio ({}) does not match render aspect ratio ({}). This may lead to "
        "distorted renders.",
        scene_.GetCamera().GetAspectRatio(), renderAspectRatio);
}

Ray BasicTracer::GeneratePerspectiveRay(float u, float v, float aspectRatio) const noexcept
{
    const auto& cam = scene_.GetCamera();

    float fovScale = tanf(cam.GetFov() * 0.5f);
    float px = (2.0f * u - 1.0f) * fovScale * aspectRatio;
    float py = (2.0f * v - 1.0f) * fovScale;

    glm::vec4 rayOriginCameraSpace = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    glm::vec4 rayDirectionCameraSpace = glm::normalize(glm::vec4(px, -py, -1.0f, 0.0f));

    glm::vec4 rayOriginWorldSpace = cam.GetCameraToWorld() * rayOriginCameraSpace;
    glm::vec4 rayDirectionWorldSpace = cam.GetCameraToWorld() * rayDirectionCameraSpace;

    return {glm::vec3(rayOriginWorldSpace), glm::normalize(glm::vec3(rayDirectionWorldSpace))};
}

Ray BasicTracer::GenerateOrthogonalRay(float u, float v, float aspectRatio) const noexcept
{
    const auto& cam = scene_.GetCamera();

    float px = (2.0f * u - 1.0f) * cam.GetOrthogonalSize().x * aspectRatio / cam.GetAspectRatio();
    float py = (2.0f * v - 1.0f) * cam.GetOrthogonalSize().y;

    glm::vec4 rayOriginCameraSpace = glm::vec4(px, -py, 0.0f, 1.0f);
    glm::vec4 rayDirectionCameraSpace = glm::normalize(glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));

    glm::vec4 rayOriginWorldSpace = cam.GetCameraToWorld() * rayOriginCameraSpace;
    glm::vec4 rayDirectionWorldSpace = cam.GetCameraToWorld() * rayDirectionCameraSpace;

    return {glm::vec3(rayOriginWorldSpace), glm::normalize(glm::vec3(rayDirectionWorldSpace))};
}

Ray BasicTracer::GenerateCameraRay(float u, float v, float aspectRatio) const noexcept
{
    const auto& cam = scene_.GetCamera();
    return cam.GetType() == Scene::Camera::Type::Perspective
               ? GeneratePerspectiveRay(u, v, aspectRatio)
               : GenerateOrthogonalRay(u, v, aspectRatio);
}

BasicTracer::BasicTracer(std::shared_ptr<RenderBuffer> imageBuffer, const Scene::Scene& scene)
    : imageBuffer_(std::move(imageBuffer)), scene_(scene)
{
    const auto aspect = static_cast<float>(imageBuffer_->GetWidth()) /
                        static_cast<float>(imageBuffer_->GetHeight());
    CheckCameraAspectRatio(aspect);
}

glm::vec3 BasicTracer::GenerateRandomDirection() noexcept
{
    std::uniform_real_distribution<float> phi(0.0f, 2.0f * glm::pi<float>());
    std::uniform_real_distribution<float> theta(-glm::half_pi<float>(), glm::half_pi<float>());

    const float u = phi(rng);
    const float v = theta(rng);

    return glm::normalize(glm::vec3(cosf(u) * cosf(v), sinf(v), sinf(u) * cosf(v)));
}

glm::vec3 BasicTracer::GenerateHemisphereDirection(const glm::vec3& normal) noexcept
{
    const auto dir = GenerateRandomDirection();
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

Ray BasicTracer::ReflectSpecular(
    const RayHit& hit, const glm::vec3& normal, float roughness) noexcept
{
    const auto dirDiffuse = glm::normalize(GenerateHemisphereDirection(normal));
    const auto dirSpecular = glm::normalize(glm::reflect(hit.direction, normal));
    const auto dir = glm::normalize(glm::mix(dirSpecular, dirDiffuse, roughness));
    const auto pos = hit.origin + hit.direction * hit.distance + pulloutEpsilon * normal;
    return {pos, dir};
}

Ray BasicTracer::ReflectDiffuse(const RayHit& hit, const glm::vec3& normal) noexcept
{
    const auto dir = glm::normalize(GenerateHemisphereDirection(normal));
    const auto pos = hit.origin + hit.direction * hit.distance + pulloutEpsilon * normal;
    return {pos, dir};
}

glm::vec3 BasicTracer::ProcessRay(const Ray& ray) noexcept
{
    constexpr int bounceArraySize = 16;
    auto maxBounces = Config::GetConfig().rendering.maxBounces;

    std::uniform_real_distribution<float> randomFloat(0.0f, 1.0f);

    Bounce bounces[bounceArraySize];
    unsigned bounceCount = 0;
    Ray newRay, currentRay = ray;
    glm::vec3 totalEnergy(1.0f);

    for (bounceCount = 0; bounceCount < maxBounces; ++bounceCount)
    {
        const auto hit = BasicBackend::IntersectScene(currentRay, scene_, false);
        if (!hit.has_value())
        {
            ++bounceCount;
            break;
        }

        const auto geom = RayHitGeometryInfo(*hit);
        const auto& matRef = hit->meshInstance->GetMaterial();
        auto mat = matRef.SampleMaterial(geom.TexCoord0);
        mat.baseColor = mat.baseColor * 0.96f + 0.04f;

        bounces[bounceCount].incomingRay = ray;
        bounces[bounceCount].hitInfo = *hit;
        bounces[bounceCount].materialPoint = mat;

        const auto fresnel = BasicBackend::BRDF::FresnelSchlick(
            glm::max(glm::dot(-currentRay.direction, geom.Normal), 0.0f), mat.metallic);

        const auto normal = ComputeNormal(geom.Normal, mat.normal);
        float energyTransfer = 1.0f;
        if (randomFloat(rng) > fresnel)
        {
            newRay = ReflectDiffuse(*hit, normal);
            energyTransfer = BasicBackend::BRDF::BRDF(
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
    for (unsigned i = 0; i < bounceCount; i++)
    {
        const auto& bounce = bounces[i];

        for (const auto& light : scene_.GetLights())
        {
            // For now we assume that all lights are point lights
            const auto lightPos =
                light.GetPosition() + GenerateRandomDirection() * light.GetPointSize();
            const auto lightColor = light.GetColor();
            const glm::vec3 lightDir = lightPos - bounce.hitInfo.worldPosition;
            const Ray shadowRay(bounce.hitInfo.worldPosition + lightDir * pulloutEpsilon, lightDir);
            const auto shadowHit = BasicBackend::IntersectScene(shadowRay, scene_, true);

            float metalness = bounce.materialPoint.metallic;
            float roughness = bounce.materialPoint.roughness;
            float lightDivisor = 1000.0f;
            float brdf = BasicBackend::BRDF::BRDF(glm::normalize(lightDir),
                -bounce.incomingRay.direction, bounce.effectiveNormal, roughness, metalness);
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

void BasicTracer::RenderBlock(const Block& block)
{
    const auto& config = Config::GetConfig();
    const auto imageWidth = config.image.width;
    const auto imageHeight = config.image.height;
    const auto imageSamples = config.image.samples;
    const auto aspectRatio = static_cast<float>(imageWidth) / static_cast<float>(imageHeight);
    const auto pixelSize = glm::vec2(1.0f) / glm::vec2(imageWidth, imageHeight);

    std::uniform_real_distribution<float> xDist(0.0f, pixelSize.x);
    std::uniform_real_distribution<float> yDist(0.0f, pixelSize.y);

    auto renderPixel = [&](int x, int y)
    {
        const auto baseU = static_cast<float>(x) * pixelSize.x;
        const auto baseV = static_cast<float>(y) * pixelSize.y;
        glm::vec3 color(0.0f);

        for (unsigned s = 0; s < imageSamples; ++s)
        {
            const auto u = baseU + xDist(rng);
            const auto v = baseV + yDist(rng);
            const Ray ray = GenerateCameraRay(u, v, aspectRatio);
            color += ProcessRay(ray);
        }
        color /= static_cast<float>(imageSamples);
        imageBuffer_->SetPixel(x, y, color);
    };

    for (int y = block.offset.y; y < block.offset.y + block.size.y; ++y)
    {
        for (int x = block.offset.x; x < block.offset.x + block.size.x; ++x)
        {
            renderPixel(x, y);
        }
    }
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
    const auto numThreads = config.rendering.numThreads ? config.rendering.numThreads
                                                        : std::thread::hardware_concurrency();

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
    stats.raysTraced = 0;  // TODO
    return stats;
}

bool BasicTracer::IsDone() const
{
    return initialQueueSize_ &&  // A render was started
           blocks_.empty() &&    // No more blocks to render
           workers_.empty();     // All worker threads have finished
}
