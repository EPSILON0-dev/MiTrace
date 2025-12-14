#pragma once

#include <GLFW/glfw3.h>

#include <memory>
#include <string>

#include "Common/ThreadSafeVar.hpp"
#include "GUI/GL_Texture.hpp"
#include "Trace/ImageBuffer.hpp"

namespace GUI
{
class Window;
}

class GUI::Window
{
   private:
    GLFWwindow* window_ = nullptr;
    ThreadSafeVariable<std::string> statusMessage_;
    ThreadSafeVariable<float> progress_;
    ThreadSafeVariable<const Trace::ImageBuffer*> cpuTexture_;
    ThreadSafeVariable<float> textureRefreshInterval_{0.0f};
    std::unique_ptr<GLTexture> texture_;

   public:
    Window(int width, int height, const char* title);
    ~Window();

    void Run();

    void SetProgress(float progress) { this->progress_.Set(progress); }
    void SetStatusMessage(const char* message) { statusMessage_.Set(message); }
    void SetTexture(Trace::ImageBuffer& texture) { cpuTexture_.Set(&texture); }
    void SetTextureRefreshInterval(float seconds) { textureRefreshInterval_.Set(seconds); }

   private:
    void Init(int w, int h, const char* t);
    void Shutdown();
    void Frame();

    void RefreshTextureIfNeeded();
    void MainWindow();
};
