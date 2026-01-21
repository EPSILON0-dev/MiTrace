#pragma once

#include <memory>
#include "Trace/Camera.hpp"
#include "Trace/Ray.hpp"
#include "Trace/RayHit.hpp"
#include "Trace/RenderBuffer.hpp"
#include "Trace/Scene.hpp"

class Trace
{
   private:
    struct BucketJob
    {
        glm::ivec2 offset;
        glm::ivec2 size;
        int samples;
    };
    using BucketRays = std::vector<Ray>;
    using BucketResult = std::vector<glm::vec3>;

   private:
    // Scene components
    std::shared_ptr<RenderBuffer> imageBuffer_;
    const Camera& camera_;
    const Scene& scene_;

   private:
    std::optional<RayHit> TraceScene(
        const Ray& ray, const Scene& scene, bool anyHit = false) noexcept;
    std::vector<BucketJob> GenerateJobs() noexcept;
    void GenerateBucket(
        BucketRays& bucket, glm::ivec2 offset, glm::ivec2 size, int samples) noexcept;
    glm::vec3 ProcessRay(const Ray& ray) noexcept;
    void ProcessBucket(const BucketRays& bucket, BucketResult& result) noexcept;
    void WriteBucketToImage(const BucketJob& job, const BucketResult& result) noexcept;
    void NormalizeImage() noexcept;

   public:
    Trace(std::shared_ptr<RenderBuffer> imageBuffer, const Camera& camera, const Scene& scene) noexcept
        : imageBuffer_(imageBuffer), camera_(camera), scene_(scene) {};
    ~Trace() = default;

    void RenderNormal();
    void RenderBucketted();
};
