#include <spdlog/spdlog.h>

#include <chrono>
#include <memory>
#include <thread>

#include "Common/ScopeTimer.hpp"
#include "Config/Config.hpp"
#include "GUI/GUI.hpp"
#include "Loader/GLTF_Loader.hpp"
#include "Trace/RenderBuffer.hpp"
#include "Trace/Trace.hpp"

bool terminateRender = false;

void RenderThread(std::shared_ptr<RenderBuffer> texture, const char* gltfFilePath)
{
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::steady_clock;

    GLTF_Loader loader(gltfFilePath);
    const auto camera = loader.LoadSceneCamera(0);
    const auto scene = loader.LoadScene(0);
    float renderDuration = 0.0f;

    {
        ScopeTimerPrint timer("### Warmup Render ###");
        Trace(texture, camera, scene).RenderNormal();
    }

    {
        ScopeTimerPrint timer("### Render Bucketted ###");
        Trace(texture, camera, scene).RenderBucketted();
    }

    {
        ScopeTimerPrint timer("### Render Normal ###");
        Trace(texture, camera, scene).RenderNormal();
    }

    spdlog::info("Render completed in {:.2f} seconds", renderDuration);
    loader.Cleanup();
}

int main(int argc, char** argv)
{
    Config::Instance().LoadConfig(argc, argv);
    const auto& config = Config::Instance().GetConfig();
    const auto verbose = Config::Instance().IsVerbose();
    spdlog::set_level(verbose ? spdlog::level::debug : spdlog::level::info);

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
