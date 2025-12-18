#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <thread>

#include "Common/Logger.pch.hpp"  // IWYU pragma: keep
#include "GUI/GUI.hpp"
#include "Loader/GLTF_Loader.hpp"
#include "Trace/Scene.hpp"
#include "Trace/Trace.hpp"

// TODO : Add global kill switch for render thread

void RenderThread(GUI::Window& gui, Trace::ImageBuffer& texture, const char* gltfFilePath)
{
    Camera cam(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, -1.0f), 45.0f);

    spdlog::set_level(static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL));

    GLTF_Loader loader(gltfFilePath);
    {
        glm::mat4 transform(1.0f);
        transform = glm::rotate(transform, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        transform = glm::rotate(transform, glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f));

        Scene scene;
        auto instances = loader.LoadSceneMeshes(0, transform);
        scene.AddMeshInstances(instances);

        const int texSize = 100;
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

            gui.SetProgress(y / 99.0f);
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
    Trace::ImageBuffer tex(100, 100);

    std::thread renderThread(RenderThread, std::ref(gui), std::ref(tex), argv[1]);

    gui.SetTexture(tex);
    gui.SetTextureRefreshInterval(1.0f / 5.0f);
    gui.Run();
    renderThread.join();
}
