#include <spdlog/spdlog.h>
#include <chrono>
#include <thread>

#include "Common/Logger.pch.hpp"  // IWYU pragma: keep
#include "Config/Config.hpp"
#include "GUI/GUI.hpp"
#include "Loader/GLTF_Loader.hpp"
#include "Trace/Dispatch.hpp"

bool terminateRender = false;

void RenderThread(GUI::Window& gui, Trace::ImageBuffer& texture, const char* gltfFilePath)
{
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::steady_clock;

    spdlog::set_level(static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL));
    GLTF_Loader loader(gltfFilePath);
    const auto camera = loader.LoadSceneCamera(0);
    const auto scene = loader.LoadScene(0);

    steady_clock::time_point startTime = steady_clock::now();
    Dispatch dispatcher(texture, camera, scene);
    dispatcher.StartThreads();

    gui.SetStatusMessage("Rendering...");
    while (!dispatcher.IsRenderComplete())
    {
        if (terminateRender)
        {
            dispatcher.KillThreads();
            SPDLOG_INFO("Render terminated by user.");
            break;
        }

        gui.SetProgress(dispatcher.GetProgress());
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    steady_clock::time_point endTime = steady_clock::now();

    dispatcher.JoinThreads();

    auto renderDuration =
        static_cast<float>(duration_cast<milliseconds>(endTime - startTime).count() / 1000.0f);
    SPDLOG_INFO("Render completed in {:.2f} seconds", renderDuration);
    gui.SetStatusMessage(
        std::format("Render complete. Time taken: {:.2f} seconds.", renderDuration).c_str());
    gui.SetProgress(1.0f);

    loader.Cleanup();
}

int main(int argc, char** argv)
{
    Config::Instance().LoadFromArgs(argc, argv);
    const auto& config = Config::Instance();

    GUI::Window gui(800, 600, "Render View");
    Trace::ImageBuffer tex(config.ImageWidth(), config.ImageHeight());

    const auto file = config.InputFilePath().c_str();
    std::thread renderThread(RenderThread, std::ref(gui), std::ref(tex), file);

    gui.SetTexture(tex);
    gui.SetTextureRefreshInterval(1.0f / 5.0f);
    gui.Run();

    terminateRender = true;
    renderThread.join();
}
