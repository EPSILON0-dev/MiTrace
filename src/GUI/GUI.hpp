#pragma once

#include <GLFW/glfw3.h>

#include <atomic>
#include <memory>
#include <thread>

#include "GUI/GL_Texture.hpp"
#include "Trace/RenderBuffer.hpp"

namespace GUI
{
class Window;
}

class GUI::Window
{
   private:
    GLFWwindow* window_ = nullptr;
    std::shared_ptr<RenderBuffer> cpuTexture_;
    std::atomic<float> textureRefreshInterval_{0.0f};
    std::atomic<bool> shouldRefreshTexture_{true};
    std::thread textureRefreshThread_;
    std::atomic<bool> exitTextureRefreshThread_{false};
    std::unique_ptr<GLTexture> texture_;

   public:
    Window(int width, int height, const char* title);
    ~Window();

    void Run();

    void SetTexture(const std::shared_ptr<RenderBuffer>& texture) { cpuTexture_ = texture; }
    void SetTextureRefreshInterval(float seconds) { textureRefreshInterval_ = seconds; }
    
    private:
    void Init(int w, int h, const char* t);
    void Shutdown();
    void Frame();
    
    void RefreshTextureIfNeeded();
    void MainWindow();
    
    void RefreshThreadFunc();
};
