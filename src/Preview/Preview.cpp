#include "GLFW/glfw3.h"
#ifdef ENABLE_PREVIEW_GUI
#include "Preview.hpp"

#include <spdlog/spdlog.h>

#include <chrono>
#include <stdexcept>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

using namespace Preview;

PreviewTexture::PreviewTexture(unsigned width, unsigned height, const unsigned char* rgbPixelsU8)
    : width_(width), height_(height)
{
    glGenTextures(1, &textureID_);
    glBindTexture(GL_TEXTURE_2D, textureID_);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, static_cast<GLsizei>(width_),
        static_cast<GLsizei>(height_), 0, GL_RGB, GL_UNSIGNED_BYTE, rgbPixelsU8);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
}

PreviewTexture::~PreviewTexture()
{
    glDeleteTextures(1, &textureID_);
}

PreviewWindow::PreviewWindow(const std::shared_ptr<RenderBuffer>& renderTexture)
    : renderBuffer_(renderTexture)
{
    using std::chrono::milliseconds;
    using std::this_thread::sleep_for;

    if (!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window_ = glfwCreateWindow(kDefaultWidth, kDefaultHeight, kDefaultTitle, nullptr, nullptr);
    if (!window_)
    {
        glfwTerminate();
        throw std::runtime_error("Failed to create window");
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui::GetIO().IniFilename = nullptr;  // Disable saving .ini file

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    texture_ = std::make_unique<PreviewTexture>(1, 1, nullptr);
    spdlog::info("Opened preview window");
}

PreviewWindow::~PreviewWindow()
{
    exitTextureRefreshThread_ = true;
    textureRefreshThread_.join();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (window_)
    {
        glfwDestroyWindow(window_);
    }
    glfwTerminate();
    spdlog::info("Closed preview window");
}

void PreviewWindow::Open()
{
    if (!renderBuffer_) throw std::runtime_error("No texture set for GUI window");
    textureRefreshThread_ = std::thread(&PreviewWindow::RefreshThreadFunc, this);

    while (!glfwWindowShouldClose(window_))
    {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        RefreshTextureIfNeeded();
        DrawContents();

        ImGui::Render();

        int displayW;
        int displayH;
        glfwGetFramebufferSize(window_, &displayW, &displayH);
        glViewport(0, 0, displayW, displayH);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window_);
    }
}

void PreviewWindow::Close()
{
    glfwSetWindowShouldClose(window_, true);
}

void PreviewWindow::RefreshTextureIfNeeded()
{
    if (!shouldRefreshTexture_) return;
    shouldRefreshTexture_ = false;

    const auto& cpuTex = renderBuffer_.get();
    if (cpuTex)
    {
        auto pixels = cpuTex->GetPixelsRGB8();
        texture_ =
            std::make_unique<PreviewTexture>(cpuTex->GetWidth(), cpuTex->GetHeight(), pixels.get());
    }
}

void PreviewWindow::DrawContents()
{
    ImGuiViewport* vp = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("RootWindow", nullptr, flags);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    ImVec2 avail = ImGui::GetContentRegionAvail();

    ImGui::BeginChild("MainView", ImVec2(avail.x, avail.y - 1), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImVec2 region = ImGui::GetContentRegionAvail();
    ImVec2 boxSize(region.x * 0.95f, region.y * 0.95f);

    boxSize =
        ImVec2(std::min(boxSize.x, boxSize.y * (static_cast<float>(texture_->GetWidth()) /
                                                   static_cast<float>(texture_->GetHeight()))),
            std::min(boxSize.y, boxSize.x * (static_cast<float>(texture_->GetHeight()) /
                                                static_cast<float>(texture_->GetWidth()))));

    ImVec2 cursor = ImGui::GetCursorPos();
    ImGui::SetCursorPos(
        ImVec2(cursor.x + (region.x - boxSize.x) * 0.5f, cursor.y + (region.y - boxSize.y) * 0.5f));

    auto texRef = static_cast<ImTextureID>(texture_->GetID());
    ImGui::Image(texRef, boxSize, ImVec2(0, 0), ImVec2(1, 1));

    ImGui::EndChild();

    ImGui::PopStyleVar();
    ImGui::End();
}

void PreviewWindow::RefreshThreadFunc()
{
    using std::chrono::milliseconds;
    using std::this_thread::sleep_for;

    while (!exitTextureRefreshThread_)
    {
        shouldRefreshTexture_ = true;
        sleep_for(milliseconds(static_cast<int>(kDefaultTextureRefreshInterval * 1000)));
    }
}
#endif
