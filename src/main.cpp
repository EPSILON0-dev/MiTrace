#include <spdlog/spdlog.h>

#include <chrono>
#include <filesystem>
#include <format>
#include <memory>
#include <thread>

#include "Loader/Config.hpp"
#include "Loader/GLTF.hpp"
#include "Preview/Preview.hpp"
#include "Tracer/Backends/Basic/Tracer.hpp"
#include "Tracer/RenderBuffer.hpp"
#include "Tracer/Tracer.hpp"

void StatThread(const Tracer& tracer, bool& shouldStop)
{
    using std::chrono::milliseconds;
    using std::chrono::seconds;
    using std::chrono::system_clock;
    using std::chrono::time_point;
    using std::this_thread::sleep_for;

    const float updatePercentage = 10.0f;
    const int updateIntervalMs = 1000;
    const int maxUpdateIntervalMs = 30000;
    const int minUpdateIntervalMs = 3000;

    time_point<system_clock> lastUpdateTime = system_clock::now();
    float lastProgress = 0.0f;
    while (!shouldStop && !tracer.IsDone())
    {
        sleep_for(milliseconds(updateIntervalMs));
        auto now = system_clock::now();
        if (now - lastUpdateTime < milliseconds(minUpdateIntervalMs)) continue;

        auto stats = tracer.GetStats();
        if (stats.progress - lastProgress >= updatePercentage ||
            now - lastUpdateTime >= milliseconds(maxUpdateIntervalMs) || true)
        {
            spdlog::info(
                "Progress: {:.2f}%, Time Elapsed: {:.1f}s, Estimated Time Remaining: {:.1f}s",
                stats.progress * 100.0f, stats.timeElapsed, stats.estimatedTimeRemaining);
            lastProgress = stats.progress;
            lastUpdateTime = now;
        }
    }
}

void RenderThread(std::shared_ptr<RenderBuffer> texture, const char* gltfFilePath, bool& shouldStop)
{
    using std::chrono::milliseconds;
    using std::chrono::seconds;
    using std::chrono::steady_clock;
    using std::chrono::system_clock;

    std::optional<Scene::Scene> scene;
    {
        Loader::GLTF loader(gltfFilePath);
        const auto loaderCamera = loader.LoadSceneCamera(0);
        const auto loaderScene = loader.LoadScene(0);
        const auto camera = Scene::Camera(loaderCamera);
        scene = Scene::Scene(loaderScene);
        scene->SetCamera(camera);
    }

    auto tracerBackend = BasicTracer(texture, scene.value());
    auto& tracer = dynamic_cast<Tracer&>(tracerBackend);

    spdlog::info("Starting render...");
    std::thread statThread(StatThread, std::cref(tracer), std::ref(shouldStop));
    tracer.StartRender();
    tracer.WaitForRender();
    shouldStop = true;
    statThread.join();
    spdlog::info("Render completed.");

    auto now = std::chrono::system_clock::now();
    auto filename = std::format("outputs/render_{:%d%m%Y_%H%M}.png", now);
    texture->SaveToFile(filename);
    spdlog::info("Saved rendered image to '{}'", filename);
}

int main(int argc, char** argv)
{
    Config::Instance().LoadConfig(argc, argv);
    const auto& config = Config::GetConfig();
    const auto verbose = Config::Instance().IsVerbose();
    const auto veryVerbose = Config::Instance().IsVeryVerbose();
    spdlog::set_level(veryVerbose ? spdlog::level::trace
                                  : (verbose ? spdlog::level::debug : spdlog::level::info));

    if (std::filesystem::exists("outputs"))
    {
        if (!std::filesystem::is_directory("outputs"))
        {
            spdlog::critical("'outputs' exists but is not a directory.");
            return EXIT_FAILURE;
        }
    }
    else
    {
        std::filesystem::create_directory("outputs");
        spdlog::info("Created 'outputs' directory.");
    }

    bool shouldStop = false;
    std::shared_ptr<RenderBuffer> tex =
        std::make_shared<RenderBuffer>(config.image.width, config.image.height);
    const auto file = config.input.filename.c_str();
    std::thread renderThread(RenderThread, tex, file, std::ref(shouldStop));

    if (Config::Instance().IsPreviewEnabled())
    {
        Preview::PreviewWindow(tex).Open();
        shouldStop = true;
    }

    renderThread.join();
}
