#pragma once

#include <memory>
#include <random>

#include "Scene/Scene.hpp"
#include "Trace/Ray.hpp"
#include "Trace/RayHit.hpp"
#include "Trace/RenderBuffer.hpp"

class Trace
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
    // Scene components
    std::shared_ptr<RenderBuffer> imageBuffer_;
    const Scene::Scene& scene_;
    std::random_device rd_;
    std::mt19937 rng_;

   private:
    Ray GenerateCameraRay(float u, float v, float aspectRatio) const noexcept;
    std::optional<RayHit> TraceScene(
        const Ray& ray, const Scene::Scene& scene, bool anyHit = false) noexcept;
    glm::vec3 GenerateHemisphereDirection(const glm::vec3& normal) noexcept;
    glm::vec3 ComputeNormal(const glm::vec3& surfNorm, const glm::vec3& texNorm) noexcept;
    Ray ReflectSpecular(const RayHit& hit, const glm::vec3& normal, float roughness) noexcept;
    Ray ReflectDiffuse(const RayHit& hit, const glm::vec3& normal) noexcept;
    glm::vec3 ProcessRay(const Ray& ray) noexcept;
    void RenderBlock(const Block& block);

   public:
    Trace(std::shared_ptr<RenderBuffer> imageBuffer, const Scene::Scene& scene);
    ~Trace() = default;

    void Render();
};
