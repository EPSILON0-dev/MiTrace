#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <memory>
#include <thread>

#include "GUI/GUI.hpp"
#include "Trace/GLTF_Loader.hpp"
#include "Trace/MeshInstance.hpp"
#include "Trace/Scene.hpp"
#include "Trace/Trace.hpp"

// TODO : Add global kill switch for render thread

void RenderThread(GUI::Window& gui, Trace::ImageBuffer& texture, const char* gltfFilePath)
{
    Camera cam(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, -1.0f), 45.0f);

    GLTF_Loader loader(gltfFilePath);
    std::shared_ptr<Mesh> meshPtr = loader.LoadMesh(0, 0);
    auto &mesh = *meshPtr;

    glm::mat4 meshMat(1.0f);
    meshMat = glm::rotate(meshMat, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    meshMat = glm::rotate(meshMat, glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    MeshInstance meshInstance(meshPtr, meshMat);

    Scene scene;
    scene.AddMeshInstance(meshInstance);

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
