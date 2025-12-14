#include <thread>

#include "GUI/GUI.hpp"
#include "Scene/Scene.hpp"
#include "Trace/Trace.hpp"

void RenderThread(GUI::Window& gui, Trace::ImageBuffer& texture)
{
    Camera cam(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, -1.0f), 45.0f);

    Scene scene;
    scene.AddSphere(glm::vec3(0.0f, 1.0f, -3.0f), 1.0f);
    scene.AddSphere(glm::vec3(0.0f, -1000.0f, 0.0f), 1000.0f);

    // Generate the texture
    const int texSize = 100;
    const float aspectRatio = 1.0f;
    for (int y = 0; y < texSize; ++y)
    {
        for (int x = 0; x < texSize; ++x)
        {
            Ray ray = cam.GenerateRay((static_cast<float>(x) + 0.5f) / static_cast<float>(texSize),
                (static_cast<float>(y) + 0.5f) / static_cast<float>(texSize), 1.0f);
            int hit = Trace::TraceScene(ray, scene) * 255;
            texture.SetPixel(x, y, hit, hit, hit);
        }

        gui.SetProgress(y / 99.0f);
        gui.SetStatusMessage(("Rendering... "));
    }

    gui.SetStatusMessage("Render complete.");
}

int main()
{
    GUI::Window gui(800, 600, "Render View");
    Trace::ImageBuffer tex(100, 100);

    std::thread renderThread(RenderThread, std::ref(gui), std::ref(tex));

    gui.SetTexture(tex);
    gui.SetTextureRefreshInterval(1.0f / 10.0f);
    gui.Run();
    renderThread.join();
}
