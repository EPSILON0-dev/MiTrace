// FIXME File needs fixes

/**
 * @file Trace.hpp
 *
 * The tracer entry point, responsible for rendering a scene from a camera's
 * perspective into a render buffer.
 *
 * Interface:
 *  - Trace(imageBuffer, camera, scene) : Creates the tracer instance.
 *  - RenderNormal() : Renders the scene using normal path tracing.
 */
#pragma once

#include <memory>
#include <random>

#include "Scene/Camera.hpp"
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

    /*
        struct BucketJob
        {
            glm::ivec2 offset;
            glm::ivec2 size;
            int samples;
        };
        using BucketRays = std::vector<Ray>;
        using BucketResult = std::vector<glm::vec3>;
    */

   private:
    // Scene components
    std::shared_ptr<RenderBuffer> imageBuffer_;
    const Scene::Camera& camera_;
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
    /*
        std::vector<BucketJob> GenerateJobs() noexcept;
        void GenerateBucket(
            BucketRays& bucket, glm::ivec2 offset, glm::ivec2 size, int samples) noexcept;
        void ProcessBucket(const BucketRays& bucket, BucketResult& result) noexcept;
        void WriteBucketToImage(const BucketJob& job, const BucketResult& result) noexcept;
        void NormalizeImage() noexcept;
    */

   public:
    Trace(std::shared_ptr<RenderBuffer> imageBuffer, const Scene::Camera& camera,
        const Scene::Scene& scene) noexcept;
    ~Trace() = default;

    void RenderNormal();
    // void RenderBucketted();
};
