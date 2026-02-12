#pragma once

#include <chrono>
#include <memory>
#include <mutex>
#include <queue>
#include <random>

#include "Ray.hpp"
#include "RayHit.hpp"
#include "Scene/Scene.hpp"
#include "Tracer/RenderBuffer.hpp"
#include "Tracer/Tracer.hpp"

class BasicTracer : public Tracer
{
   private:
    struct Bounce
    {
        Ray incomingRay;
        Ray outgoingRay;
        RayHit hitInfo;
        Scene::Material::MaterialPoint materialPoint;
        glm::vec3 effectiveNormal;
        glm::vec3 energyTransfer;
    };

    struct Block
    {
        glm::ivec2 offset;
        glm::ivec2 size;
    };

   private:
    std::shared_ptr<RenderBuffer> imageBuffer_;
    const Scene::Scene& scene_;
    std::random_device rd_;
    std::mt19937 rng_;
    std::queue<Block> blocks_;
    unsigned int initialQueueSize_ = 0;
    std::mutex blockMutex_;
    std::vector<std::thread> workers_;
    std::chrono::time_point<std::chrono::system_clock> startTime_;
    bool renderKilled_ = false;

   private:
    Ray GenerateCameraRay(float u, float v, float aspectRatio) const noexcept;
    std::optional<RayHit> TraceScene(const Ray& ray, bool anyHit = false) noexcept;
    glm::vec3 GenerateHemisphereDirection(const glm::vec3& normal) noexcept;
    Ray ReflectSpecular(const RayHit& hit, const glm::vec3& normal, float roughness) noexcept;
    Ray ReflectDiffuse(const RayHit& hit, const glm::vec3& normal) noexcept;
    glm::vec3 ProcessRay(const Ray& ray) noexcept;
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
