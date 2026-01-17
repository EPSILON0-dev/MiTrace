#include "Dispatch.hpp"

#include "Config/Config.hpp"
#include "Trace/ImageBuffer.hpp"
#include "Trace/Trace.hpp"

void Dispatch::InitializeSectors()
{
    blockQueue_.clear();
    const auto sectorSize = Config::Instance().SectorSize();
    const auto width = imageBuffer_.GetWidth();
    const auto height = imageBuffer_.GetHeight();

    for (uint32_t y = 0; y < height; y += sectorSize)
    {
        for (uint32_t x = 0; x < width; x += sectorSize)
        {
            RenderSector sector;
            sector.xStart = x;
            sector.yStart = y;
            sector.width = std::min(sectorSize, width - x);
            sector.height = std::min(sectorSize, height - y);
            blockQueue_.push_back(sector);
        }
    }

    initialBlockCount_ = blockQueue_.size();
}

std::optional<Dispatch::RenderSector> Dispatch::GetNextSector()
{
    std::lock_guard<std::mutex> lock(blockQueueMutex_);

    if (blockQueue_.empty()) return std::nullopt;

    RenderSector sector = blockQueue_.back();
    blockQueue_.pop_back();
    return sector;
}

void Dispatch::DrawRenderMark(const RenderSector& sector)
{
    const constexpr auto colorA = glm::u16vec4(255, 255, 100, 255);
    const constexpr auto colorB = glm::u16vec4(0, 0, 0, 255);
    const auto borderWidth = 2;

    for (uint32_t y = 0; y < sector.height; ++y)
    {
        for (uint32_t x = 0; x < sector.width; ++x)
        {
            bool isBorder = x < borderWidth || x >= sector.width - borderWidth;
            isBorder |= y < borderWidth || y >= sector.height - borderWidth;

            glm::u8vec4 color = (isBorder) ? colorA : colorB;
            imageBuffer_.SetPixel(sector.xStart + x, sector.yStart + y, color.r, color.g, color.b);
        }
    }
}

void Dispatch::WorkerThreadFunc()
{
    const auto samples = Config::Instance().SamplesPerPixel();
    const auto aspectRatio =
        static_cast<float>(imageBuffer_.GetWidth()) / static_cast<float>(imageBuffer_.GetHeight());

    while (!shouldTerminate_)
    {
        auto sectorOpt = GetNextSector();
        if (!sectorOpt.has_value()) break;
        RenderSector sector = sectorOpt.value();
        DrawRenderMark(sector);

        // Render the sector
        for (uint32_t y = 0; y < sector.height; ++y)
        {
            for (uint32_t x = 0; x < sector.width; ++x)
            {
                // Generate ray for pixel (x, y)
                Ray ray = camera_.GenerateRay((static_cast<float>(x + sector.xStart) + 0.5f) /
                                                  static_cast<float>(imageBuffer_.GetWidth()),
                    (static_cast<float>(y + sector.yStart) + 0.5f) /
                        static_cast<float>(imageBuffer_.GetHeight()),
                    aspectRatio);

                // Trace the ray through the scene
                glm::u16vec4 color(0);
                for (uint32_t sample = 0; sample < samples; ++sample)
                    color += Trace::TraceSample(ray, scene_);
                color /= static_cast<uint16_t>(samples);

                // Set the pixel color in the image buffer
                const auto ex = x + sector.xStart;
                const auto ey = y + sector.yStart;
                imageBuffer_.SetPixel(ex, ey, color.r, color.g, color.b);
            }
        }
    }
}

void Dispatch::StartThreads()
{
    shouldTerminate_ = false;
    InitializeSectors();

    /*
    const auto configThreadCount = Config::Instance().GetThreadCount();
    const auto threadCount =
        configThreadCount.has_value()
            ? configThreadCount.value()
            : std::thread::hardware_concurrency() / 4;  // There's no gain in using any more than
                                                        // 1/4 of available threads for rendering
                                                        // as they are mostly waiting on IO
                                                        */

    // For now locked down to 1 thread, there's no benefit to multithreading with the current
    // implementation
    const auto threadCount = 1;

    workerThreads_.reserve(threadCount);
    for (unsigned int i = 0; i < threadCount; ++i)
        workerThreads_.emplace_back(&Dispatch::WorkerThreadFunc, this);
}

void Dispatch::JoinThreads()
{
    for (auto& thread : workerThreads_)
        if (thread.joinable()) thread.join();
    workerThreads_.clear();
}

void Dispatch::KillThreads()
{
    shouldTerminate_ = true;
    JoinThreads();
}

float Dispatch::GetProgress() const
{
    std::lock_guard<std::mutex> lock(blockQueueMutex_);
    const size_t totalSectors = initialBlockCount_;
    const size_t remainingSectors = blockQueue_.size();
    return 1.0f - static_cast<float>(remainingSectors) / static_cast<float>(totalSectors);
}

bool Dispatch::IsRenderComplete() const
{
    std::lock_guard<std::mutex> lock(blockQueueMutex_);
    return blockQueue_.empty();
}
