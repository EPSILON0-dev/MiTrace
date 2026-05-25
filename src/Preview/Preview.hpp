#pragma once

#ifdef ENABLE_PREVIEW_GUI
#include <GLFW/glfw3.h>

#include <atomic>
#include <memory>
#include <thread>

#include "Tracer/RenderBuffer.hpp"

namespace Preview
{

class PreviewTexture
{
   private:
    unsigned textureID_ = 0;
    unsigned width_ = 0;
    unsigned height_ = 0;

   public:
    PreviewTexture(unsigned width, unsigned height, const unsigned char* rgbPixelsU8);
    ~PreviewTexture();

    // Disable copy
    PreviewTexture(const PreviewTexture&) = delete;
    PreviewTexture& operator=(const PreviewTexture&) = delete;
    // Allow move
    PreviewTexture(PreviewTexture&& other) noexcept = default;
    PreviewTexture& operator=(PreviewTexture&& other) noexcept = default;

    auto GetID() const { return textureID_; }
    auto GetWidth() const { return width_; }
    auto GetHeight() const { return height_; }
};

class PreviewWindow
{
   private:
    constexpr static int kDefaultWidth = 800;
    constexpr static int kDefaultHeight = 600;
    constexpr static const char* kDefaultTitle = "Render Preview";
    constexpr static float kDefaultTextureRefreshInterval = 1.0f / 5.0f;  // 5 FPS

    GLFWwindow* window_ = nullptr;
    std::shared_ptr<RenderBuffer> renderBuffer_;
    std::unique_ptr<PreviewTexture> texture_;
    std::thread textureRefreshThread_;
    std::atomic<bool> shouldRefreshTexture_{true};
    std::atomic<bool> exitTextureRefreshThread_{false};

   private:
    void DrawContents();

    void RefreshTextureIfNeeded();
    void RefreshThreadFunc();

   public:
    PreviewWindow(const std::shared_ptr<RenderBuffer>& renderTexture);
    ~PreviewWindow();

    void Open();
    void Close();
};

}  // namespace Preview
#else
#include <spdlog/spdlog.h>

// Forward declaration
class RenderBuffer;

namespace Preview
{

class PreviewWindow
{
   public:
    PreviewWindow(const std::shared_ptr<RenderBuffer>& renderTexture) { (void)renderTexture; };
    ~PreviewWindow() = default;

    void Open()
    {
        spdlog::warn("Preview GUI is disabled. Recompile with ENABLE_PREVIEW_GUI=ON to enable it.");
    }

    void Close() {}
};

}  // namespace Preview
#endif
