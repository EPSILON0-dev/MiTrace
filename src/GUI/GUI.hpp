#pragma once

#include <GLFW/glfw3.h>

#include <memory>
#include <string>
#include <thread>
#include <atomic>

#include "Common/ThreadSafeVar.hpp"
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
    ThreadSafeVariable<std::string> statusMessage_;
    std::atomic<float> progressPercent_;
    std::atomic<const RenderBuffer*> cpuTexture_; // TODO: potential ownership issue
    std::atomic<float> textureRefreshInterval_{0.0f};
    std::atomic<bool> shouldRefreshTexture_{true};
    std::thread textureRefreshThread_;
    std::atomic<bool> exitTextureRefreshThread_{false};
    std::unique_ptr<GLTexture> texture_;

   public:
    Window(int width, int height, const char* title);
    ~Window();

    void Run();

    void SetProgress(float progress) { this->progressPercent_ = progress; }
    void SetStatusMessage(const char* message) { statusMessage_.Set(message); }
    void SetTexture(RenderBuffer& texture) { cpuTexture_ = &texture; }
    void SetTextureRefreshInterval(float seconds) { textureRefreshInterval_ = seconds; }

   private:
    void Init(int w, int h, const char* t);
    void Shutdown();
    void Frame();

    void RefreshTextureIfNeeded();
    void MainWindow();
};
