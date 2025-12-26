#include <chrono>
#include <iostream>
#include <thread>

#include "Common/Logger.pch.hpp"  // IWYU pragma: keep
#include "GUI/GUI.hpp"
#include "Loader/GLTF_Loader.hpp"
#include "Trace/Trace.hpp"

bool shouldRenderDie = false;

constexpr int imageSize = 500;

void RenderThread(GUI::Window& gui, Trace::ImageBuffer& texture, const char* gltfFilePath)
{
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    using std::chrono::steady_clock;

    spdlog::set_level(static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL));
    GLTF_Loader loader(gltfFilePath);

    steady_clock::time_point startTime = steady_clock::now();
    {
        auto scene = loader.LoadScene(0);
        Camera cam = loader.LoadSceneCamera(0);

        const int texSize = imageSize;
        // const float aspectRatio = 1.0f;
        for (int y = 0; y < texSize; ++y)
        {
            for (int x = 0; x < texSize; ++x)
            {
                Ray ray =
                    cam.GenerateRay((static_cast<float>(x) + 0.5f) / static_cast<float>(texSize),
                        (static_cast<float>(y) + 0.5f) / static_cast<float>(texSize), 1.0f);
                auto color = Trace::TraceSample(ray, scene);
                texture.SetPixel(x, y, color.r, color.g, color.b);
            }

            if (shouldRenderDie)
            {
                gui.SetStatusMessage("Render aborted.");
                return;
            }

            gui.SetProgress(y / static_cast<float>(imageSize - 1));
            gui.SetStatusMessage(("Rendering... "));
        }
    }
    steady_clock::time_point endTime = steady_clock::now();

    auto renderDuration =
        static_cast<float>(duration_cast<milliseconds>(endTime - startTime).count() / 1000.0f);
    SPDLOG_INFO("Render completed in {:.2f} seconds", renderDuration);
    gui.SetStatusMessage(
        std::format("Render complete. Time taken: {:.2f} seconds.", renderDuration).c_str());

    loader.Cleanup();
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <gltf_file>" << std::endl;
        return 1;
    }

    GUI::Window gui(800, 600, "Render View");
    Trace::ImageBuffer tex(imageSize, imageSize);

    std::thread renderThread(RenderThread, std::ref(gui), std::ref(tex), argv[1]);

    gui.SetTexture(tex);
    gui.SetTextureRefreshInterval(1.0f / 5.0f);
    gui.Run();

    shouldRenderDie = true;
    renderThread.join();
}
