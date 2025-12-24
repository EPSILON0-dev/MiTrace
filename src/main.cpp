#include <iostream>
#include <thread>

#include "Common/Logger.pch.hpp"  // IWYU pragma: keep
#include "GUI/GUI.hpp"
#include "Loader/GLTF_Loader.hpp"
#include "Trace/Scene.hpp"
#include "Trace/Trace.hpp"

bool shouldRenderDie = false;

void RenderThread(GUI::Window& gui, Trace::ImageBuffer& texture, const char* gltfFilePath)
{
    spdlog::set_level(static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL));
    GLTF_Loader loader(gltfFilePath);

    {
        Scene scene;
        scene.AddMeshInstances(loader.LoadSceneMeshes(0));
        scene.AddLights(loader.LoadSceneLights(0));
        Camera cam = loader.LoadSceneCamera(0);

        const int texSize = 500;
        // const float aspectRatio = 1.0f;
        for (int y = 0; y < texSize; ++y)
        {
            for (int x = 0; x < texSize; ++x)
            {
                Ray ray =
                    cam.GenerateRay((static_cast<float>(x) + 0.5f) / static_cast<float>(texSize),
                        (static_cast<float>(y) + 0.5f) / static_cast<float>(texSize), 1.0f);
                auto color = Trace::TraceScene(ray, scene);
                texture.SetPixel(x, y, color.r, color.g, color.b);
            }

            if (shouldRenderDie)
            {
                gui.SetStatusMessage("Render aborted.");
                return;
            }

            gui.SetProgress(y / 499.0f);
            gui.SetStatusMessage(("Rendering... "));
        }
    }

    loader.Cleanup();

    gui.SetStatusMessage("Render complete.");
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <gltf_file>" << std::endl;
        return 1;
    }

    GUI::Window gui(800, 600, "Render View");
    Trace::ImageBuffer tex(500, 500);

    std::thread renderThread(RenderThread, std::ref(gui), std::ref(tex), argv[1]);

    gui.SetTexture(tex);
    gui.SetTextureRefreshInterval(1.0f / 5.0f);
    gui.Run();

    shouldRenderDie = true;
    renderThread.join();
}
