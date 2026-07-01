#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <queue>
#include <random>

#include "CLI/Config.hpp"
#include "Camera.hpp"
#include "Ray.hpp"
#include "RenderBuffer.hpp"
#include "Scene/Scene.hpp"

namespace Tracer
{

class Tracer
{
   private:
    struct Block
    {
        glm::ivec2 offset;
        glm::ivec2 size;
    };

    struct SimplifiedLight
    {
        glm::vec3 position;
        float intensity;
    };

    struct alignas(128) PathStep
    {
        Ray ray;
        glm::vec3 hitPos;
        glm::vec3 baseColor;
        float metallic;
        float roughness;
        glm::vec3 geomNormal;
        glm::vec3 normal;
        glm::vec3 energy;
        glm::vec3 emission;

        // Debug information
        float brdfIntensity;
        float pdf;
        uint32_t bvhTests;
        uint32_t triangleTests;
        uint16_t triangleIndex;
        uint16_t bvIndex;
        uint16_t meshIndex;

        bool didHit;
    };

    struct LightOutput
    {
        glm::vec3 emittedLight;
        glm::vec3 directLight;
        glm::vec3 indirectLight;
        glm::vec3 skyLight;
        glm::vec3 totalLight;
    };

   public:
    struct JobData
    {
        std::vector<PathStep> pathBuffer;
        std::mt19937 rng;
        std::uniform_real_distribution<float> randomFloat{0.0f, 1.0f};
        size_t raysTraced = 0;
        size_t samplesTraced = 0;
        float exposureMultiplier = 1.0f;
    };

    struct Stats
    {
        float progress;
        float timeElapsed;
        float estimatedTimeRemaining;
        size_t rays;
        size_t samples;
    };

   private:
    static std::random_device rd;
    std::shared_ptr<RenderBuffer> imageBuffer_;
    const Scene::Scene& scene_;
    const Camera cam_;
    std::vector<SimplifiedLight> simplifiedLights_;

    unsigned int initialQueueSize_ = 0;
    std::queue<Block> blocks_;
    std::mutex blockMutex_;

    std::atomic<size_t> raysTraced_{0};
    std::atomic<size_t> samplesTraced_{0};
    std::chrono::time_point<std::chrono::system_clock> startTime_;

    std::mutex workersMutex_;
    std::vector<std::thread> workers_;
    volatile std::atomic<bool> renderKilled_ = false;

   private:
    void PrepareLights();
    void GeneratePath(JobData& jobData, const Ray& ray, std::vector<PathStep>& pathVec,
        size_t maxBounces, float terminateEnergy = 0.01f) const noexcept;

    // glm::vec3 ProcessRayBidirectional(JobData& jobData, const Ray& ray) noexcept;
    LightOutput ProcessRayForward(
        JobData& jobData, const Ray& ray, std::vector<PathStep>& path) noexcept;
    glm::vec3 ProcessDebugRay(JobData& jobData, const Ray& ray, std::vector<PathStep>& path,
        Config::DebugMode debugMode, Config::RenderMode renderMode) noexcept;

    void RenderBlock(const Block& block);
    void WorkerThread();

   public:
    Tracer(std::shared_ptr<RenderBuffer> imageBuffer, const Scene::Scene& scene);
    ~Tracer() = default;

    void StartRender();
    void WaitForRender();
    void KillRender();
    Stats GetStats() const;
    bool IsDone() const;
};

}  // namespace Tracer
