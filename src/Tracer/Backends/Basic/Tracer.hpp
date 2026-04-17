#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <queue>
#include <random>

#include "Ray.hpp"
#include "Scene/Scene.hpp"
#include "Tracer/Backends/Basic/Camera.hpp"
#include "Tracer/RenderBuffer.hpp"
#include "Tracer/Tracer.hpp"

class BasicTracer : public Tracer
{
    using Ray = BasicBackend::Ray;
    using RayHit = BasicBackend::RayHit;

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

    struct PathStep
    {
        Ray ray;
        glm::vec3 hitPos;
        glm::vec3 baseColor;
        float metallic;
        float roughness;
        glm::vec3 normal;
        glm::vec3 energy;
        bool didHit;
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

   private:
    static std::random_device rd;
    std::shared_ptr<RenderBuffer> imageBuffer_;
    const Scene::Scene& scene_;
    const BasicBackend::BasicCamera cam_;
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
    glm::vec3 ProcessRay(JobData& jobData, const Ray& ray) noexcept;
    void RenderBlock(const Block& block);
    void WorkerThread();

   public:
    BasicTracer(std::shared_ptr<RenderBuffer> imageBuffer, const Scene::Scene& scene);
    ~BasicTracer() override = default;

    void StartRender() override;
    void WaitForRender() override;
    void KillRender() override;
    Stats GetStats() const override;
    bool IsDone() const override;
};
