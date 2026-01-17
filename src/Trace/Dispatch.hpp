#pragma once

#include "Trace/Camera.hpp"
#include "Trace/ImageBuffer.hpp"
#include "Trace/Scene.hpp"

class Dispatch
{
   public:
    struct RenderSector
    {
        uint32_t xStart;
        uint32_t yStart;
        uint32_t width;
        uint32_t height;
    };

   private:
    Trace::ImageBuffer& imageBuffer_;
    const Camera& camera_;
    const Scene& scene_;
    bool shouldTerminate_ = false;
    mutable std::mutex blockQueueMutex_;
    std::vector<RenderSector> blockQueue_;
    std::vector<std::thread> workerThreads_;
    size_t initialBlockCount_;

   private:
    void InitializeSectors();
    std::optional<RenderSector> GetNextSector();
    void DrawRenderMark(const RenderSector& sector);
    void WorkerThreadFunc();

   public:
    Dispatch(Trace::ImageBuffer& imageBuffer, const Camera& camera, const Scene& scene) noexcept
        : imageBuffer_(imageBuffer), camera_(camera), scene_(scene) {};
    ~Dispatch() = default;

    void StartThreads();
    void JoinThreads();
    void KillThreads();
    float GetProgress() const;
    bool IsRenderComplete() const;
};
