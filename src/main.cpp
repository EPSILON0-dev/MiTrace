#include <spdlog/spdlog.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <memory>
#include <stdexcept>
#include <thread>

#include "CLI/Config.hpp"
#include "Loader/GLTF.hpp"
#include "Preview/Preview.hpp"
#include "Scene/Texture.hpp"
#include "Tracer/RenderBuffer.hpp"
#include "Tracer/Tracer.hpp"

static std::optional<std::function<void()>> renderKillFunc = std::nullopt;
static std::optional<std::function<void()>> windowCloseFunc = std::nullopt;

static std::string GenerateFilename()
{
    auto now = std::chrono::system_clock::now();
    std::string filename = std::format("outputs/render_{:%d%m%Y_%H%M}.png", now);
    if (std::filesystem::exists("outputs"))
    {
        if (!std::filesystem::is_directory("outputs"))
        {
            spdlog::critical("'outputs' exists but is not a directory.");
            std::exit(EXIT_FAILURE);
        }
    }
    else
    {
        std::filesystem::create_directory("outputs");
        spdlog::info("Created 'outputs' directory.");
    }
    return filename;
}

static void StatThread(const Tracer::Tracer& tracer, volatile bool& shouldStop)
{
    using std::chrono::milliseconds;
    using std::chrono::seconds;
    using std::chrono::system_clock;
    using std::chrono::time_point;
    using std::this_thread::sleep_for;

    const int updateIntervalMs = 1000;

    time_point<system_clock> lastUpdateTime = system_clock::now();
    auto prevStats = tracer.GetStats();
    while (!shouldStop && !tracer.IsDone())
    {
        sleep_for(milliseconds(updateIntervalMs));
        auto now = system_clock::now();
        auto stats = tracer.GetStats();

        auto mrays = static_cast<float>(stats.rays - prevStats.rays) / 1'000'000.0f;
        auto msamples = static_cast<float>(stats.samples - prevStats.samples) / 1'000'000.0f;

        if (shouldStop || tracer.IsDone()) break;

        const auto logMessage = std::format(
            "Progress: {:.2f}%, Time: {:.1f}s, ETA: {:.1f}s, {:.2f}M rays/sec, {:.2f}M "
            "samples/sec\033[A",
            stats.progress * 100.0f, stats.timeElapsed, stats.estimatedTimeRemaining, mrays,
            msamples);

        if (Config::GetConfig().quiet)
            std::cout << logMessage << std::endl;
        else
            spdlog::info(logMessage);
        prevStats = stats;
        lastUpdateTime = now;
    }
}

static void RenderThread(const std::shared_ptr<RenderBuffer>& renderTarget,
    const char* gltfFilePath, volatile bool& shouldStop)
{
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::seconds;
    using std::chrono::steady_clock;
    using std::chrono::system_clock;

    const auto& cfg = Config::GetConfig();
    auto filename = (!cfg.outputFilename.empty()) ? cfg.outputFilename : GenerateFilename();
    const auto filetype = filename.substr(filename.rfind('.'));
    if (filetype != ".png" && filetype != ".jpg" && filetype != ".hdr")
    {
        spdlog::critical("Invalid output file type");
        std::exit(EXIT_FAILURE);
    }

    std::optional<Scene::Scene> scene;
    {
        auto startTime = system_clock::now();

        Loader::GLTF loader(gltfFilePath);
        const auto loaderCamera = loader.LoadSceneCamera(cfg.sceneIndex, cfg.cameraIndex);
        const auto loaderScene = loader.LoadScene(cfg.sceneIndex);
        const auto camera = Scene::Camera(loaderCamera);

        scene = Scene::Scene(loaderScene);
        scene->SetCamera(camera);

        if (!cfg.hdriOverride.empty())
        {
            spdlog::info("Overriding HDRI with '{}'", cfg.hdriOverride);
            auto hdriTexture = Scene::Texture::LoadEnvTexture(cfg.hdriOverride);
            scene->SetEnvironmentTexture(hdriTexture);
        }

        auto endTime = system_clock::now();
        auto loadDuration = duration_cast<milliseconds>(endTime - startTime).count();
        spdlog::info("Loading took {:.2f} seconds", static_cast<float>(loadDuration) / 1000.0f);
    }

    auto tracer = Tracer::Tracer(renderTarget, scene.value());

    spdlog::info("Starting render...");
    std::thread statThread(StatThread, std::cref(tracer), std::ref(shouldStop));
    auto startTime = system_clock::now();
    renderKillFunc = [&tracer]() { tracer.KillRender(); };
    tracer.StartRender();
    tracer.WaitForRender();
    renderKillFunc = std::nullopt;
    auto endTime = system_clock::now();
    auto renderDuration = duration_cast<milliseconds>(endTime - startTime).count();
    auto renderSeconds = static_cast<float>(renderDuration) / 1000.0f;
    const auto stats = tracer.GetStats();
    auto mrays = static_cast<float>(stats.rays) / 1'000'000.0f;
    auto msamples = static_cast<float>(stats.samples) / 1'000'000.0f;
    auto mraysPerSec = mrays / renderSeconds;
    auto msamplesPerSec = msamples / renderSeconds;
    spdlog::info(
        "Render completed in {:.2f} seconds, rays: {:.2f}M ({:.2f}M/s), samples: {:.2f}M "
        "({:.2f}M/s)",
        renderSeconds, mrays, mraysPerSec, msamples, msamplesPerSec);
    statThread.join();

    if (filetype == ".jpg")
        renderTarget->SaveToFileJPG(filename);
    else if (filetype == ".png")
        renderTarget->SaveToFilePNG(filename);
    else if (filetype == ".hdr")
        renderTarget->SaveToFileHDR(filename);
    else
        throw std::runtime_error("Invalid file type (failed to catch early)");

    spdlog::info("Saved rendered image to '{}'", filename);

    if (cfg.exitPreviewWhenDone && windowCloseFunc)
    {
        std::this_thread::sleep_for(seconds(2));
        (*windowCloseFunc)();
    }
}

int main(int argc, char** argv)
{
    try
    {
        Config::Instance().LoadConfig(argc, argv);
    }
    catch (std::exception& e)
    {
        spdlog::critical(e.what());
        std::exit(1);
    }

    const auto& cfg = Config::GetConfig();
    const auto verbose = cfg.verbose;
    const auto veryVerbose = cfg.veryVerbose;

    spdlog::level::level_enum logLevel = spdlog::level::info;
    if (veryVerbose)
        logLevel = spdlog::level::trace;
    else if (verbose)
        logLevel = spdlog::level::debug;
    else if (cfg.quiet)
        logLevel = spdlog::level::err;
    spdlog::set_level(logLevel);

    Config::Instance().LogConfig();

    volatile bool shouldStop = false;
    std::shared_ptr<RenderBuffer> tex =
        std::make_shared<RenderBuffer>(cfg.imageWidth, cfg.imageHeight);
    const auto file = cfg.inputFilename.c_str();
    std::thread renderThread(RenderThread, tex, file, std::ref(shouldStop));

    if (cfg.enablePreview)
    {
        auto win = Preview::PreviewWindow(tex);
        windowCloseFunc = [&]() { win.Close(); };
        win.Open();
        shouldStop = true;
        if (renderKillFunc) (*renderKillFunc)();
    }

    renderThread.join();
}
