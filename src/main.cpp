#include <spdlog/spdlog.h>

#include <chrono>
#include <filesystem>
#include <format>
#include <memory>
#include <thread>

#include "Common/ScopeTimer.hpp"
#include "GUI/GUI.hpp"
#include "Loader/Config.hpp"
#include "Loader/GLTF.hpp"
#include "Trace/RenderBuffer.hpp"
#include "Trace/Trace.hpp"

bool terminateRender = false;

void RenderThread(std::shared_ptr<RenderBuffer> texture, const char* gltfFilePath)
{
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::steady_clock;
    using std::chrono::system_clock;

    Loader::GLTF loader(gltfFilePath);

    const auto loaderCamera = loader.LoadSceneCamera(0);
    const auto loaderScene = loader.LoadScene(0);
    const auto camera = Scene::Camera(loaderCamera);
    const auto scene = Scene::Scene(loaderScene);

    loader.Cleanup();
    float renderDuration = 0.0f;

    spdlog::info("Starting render...");
    {
        ScopeTimer timer(renderDuration);
        Trace(texture, camera, scene).RenderNormal();
    }
    spdlog::info("Render completed in {:.2f} seconds", renderDuration);

    auto now = std::chrono::system_clock::now();
    auto filename = std::format("outputs/render_{:%d%m%Y_%H%M}.png", now);
    texture->SaveToFile(filename);
    spdlog::info("Saved rendered image to '{}'", filename);
}

int main(int argc, char** argv)
{
    Config::Instance().LoadConfig(argc, argv);
    const auto& config = Config::Instance().GetConfig();
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

    GUI::Window gui(800, 600, "Render View");
    std::shared_ptr<RenderBuffer> tex =
        std::make_shared<RenderBuffer>(config.image.width, config.image.height);

    const auto file = config.input.filename.c_str();
    std::thread renderThread(RenderThread, tex, file);

    gui.SetTexture(tex);
    gui.SetTextureRefreshInterval(1.0f / 5.0f);
    gui.Run();

    terminateRender = true;
    renderThread.join();
}
