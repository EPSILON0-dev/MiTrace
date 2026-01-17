#include <spdlog/spdlog.h>
#include <chrono>
#include <thread>

#include "Common/Logger.pch.hpp"  // IWYU pragma: keep
#include "Config/Config.hpp"
#include "GUI/GUI.hpp"
#include "Loader/GLTF_Loader.hpp"
#include "Trace/Trace.hpp"
#include "Common/ScopeTimer.hpp"

bool terminateRender = false;

void RenderThread(GUI::Window& gui, RenderBuffer& texture, const char* gltfFilePath)
{
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::steady_clock;

    spdlog::set_level(static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL));
    GLTF_Loader loader(gltfFilePath);
    const auto camera = loader.LoadSceneCamera(0);
    const auto scene = loader.LoadScene(0);

    float renderDuration = 0.0f;
    {
        ScopeTimer<float> timer(renderDuration);
        Trace(texture, camera, scene).Render();
    }

    spdlog::info("Render completed in {:.2f} seconds", renderDuration);
    gui.SetStatusMessage(
        std::format("Render complete. Time taken: {:.2f} seconds.", renderDuration).c_str());
    gui.SetProgress(1.0f);

    loader.Cleanup();
}

int main(int argc, char** argv)
{
    Config::Instance().LoadConfig(argc, argv);
    const auto& config = Config::Instance().GetConfig();

    GUI::Window gui(800, 600, "Render View");
    RenderBuffer tex(config.image.width, config.image.height);

    const auto file = config.input.filename.c_str();
    std::thread renderThread(RenderThread, std::ref(gui), std::ref(tex), file);

    gui.SetTexture(tex);
    gui.SetTextureRefreshInterval(1.0f / 5.0f);
    gui.Run();

    terminateRender = true;
    renderThread.join();
}
