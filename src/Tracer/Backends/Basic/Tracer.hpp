#pragma once

#include <chrono>
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <atomic>

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

    struct JobPersistentData
    {
        std::vector<PathStep> pathBuffer;
        std::mt19937 rng;
        size_t raysTraced = 0;
        size_t samplesTraced = 0;
    };

   private:
    std::atomic<size_t> raysTraced_{0};
    std::atomic<size_t> samplesTraced_{0};
    static std::random_device rd;
    std::shared_ptr<RenderBuffer> imageBuffer_;
    const Scene::Scene& scene_;
    const BasicBackend::BasicCamera cam_;
    std::queue<Block> blocks_;
    unsigned int initialQueueSize_ = 0;
    std::mutex blockMutex_;
    std::vector<std::thread> workers_;
    std::chrono::time_point<std::chrono::system_clock> startTime_;
    volatile std::atomic<bool> renderKilled_ = false;

   private:
    static Ray ReflectSpecular(JobPersistentData& jobData, const RayHit& hit,
        const glm::vec3& normal, float roughness) noexcept;
    static Ray ReflectDiffuse(
        JobPersistentData& jobData, const RayHit& hit, const glm::vec3& normal) noexcept;
    static glm::vec3 GenerateRandomDirection(JobPersistentData& jobData) noexcept;
    static glm::vec3 GenerateHemisphereDirection(
        JobPersistentData& jobData, const glm::vec3& normal) noexcept;
    void GeneratePath(JobPersistentData& jobData, const Ray& ray, std::vector<PathStep>& pathVec,
        size_t maxBounces, float terminateEnergy = 0.01f) const noexcept;
    glm::vec3 ProcessRay(JobPersistentData& jobData, const Ray& ray) noexcept;
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
