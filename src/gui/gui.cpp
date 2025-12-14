#include "gui.hpp"

#include <stdexcept>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

GUI::Window::Window(int w, int h, const char* t)
{
    Init(w, h, t);
}

GUI::Window::~Window()
{
    Shutdown();
}

void GUI::Window::Init(int w, int h, const char* t)
{
    if (!glfwInit())
    {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window_ = glfwCreateWindow(w, h, t, nullptr, nullptr);
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

    texture_ = std::make_unique<GLTexture>(1, 1, nullptr);
}

void GUI::Window::Shutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (window_)
    {
        glfwDestroyWindow(window_);
    }
    glfwTerminate();
}

void GUI::Window::Run()
{
    while (!glfwWindowShouldClose(window_))
    {
        glfwPollEvents();
        Frame();
        glfwSwapBuffers(window_);
    }
}

void GUI::Window::Frame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    RefreshTextureIfNeeded();
    MainWindow();

    ImGui::Render();

    int displayW, displayH;
    glfwGetFramebufferSize(window_, &displayW, &displayH);
    glViewport(0, 0, displayW, displayH);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GUI::Window::RefreshTextureIfNeeded()
{
    float interval = textureRefreshInterval_.Get();
    double currentTime = glfwGetTime();
    static double lastUpdateTime = 0.0;

    if (interval > 0.0f && (currentTime - lastUpdateTime) >= interval)
    {
        const CPUTexture* cpuTex = cpuTexture_.Get();
        if (cpuTex)
        {
            texture_ = std::make_unique<GLTexture>(
                cpuTex->GetWidth(), cpuTex->GetHeight(), cpuTex->GetPixels());
        }
        lastUpdateTime = currentTime;
    }
}

void GUI::Window::MainWindow()
{
    ImGuiViewport* vp = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("RootWindow", nullptr, flags);

    // Optional: edge-to-edge layout
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    float bottomHeight = 4.0f * ImGui::GetFontSize();
    ImVec2 avail = ImGui::GetContentRegionAvail();
    avail.y -= 10.0f;

    // -----------------------------
    // Top: main view (image area)
    // -----------------------------
    ImGui::BeginChild("MainView", ImVec2(avail.x, avail.y - bottomHeight - 1), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // Placeholder box for image
    ImVec2 region = ImGui::GetContentRegionAvail();
    ImVec2 boxSize(region.x * 0.95f, region.y * 0.95f);

    boxSize = ImVec2(
        std::min(boxSize.x, boxSize.y * (static_cast<float>(texture_->GetWidth()) /
                           static_cast<float>(texture_->GetHeight()))),
        std::min(boxSize.y, boxSize.x * (static_cast<float>(texture_->GetHeight()) /
                           static_cast<float>(texture_->GetWidth()))));

    ImVec2 cursor = ImGui::GetCursorPos();
    ImGui::SetCursorPos(
        ImVec2(cursor.x + (region.x - boxSize.x) * 0.5f, cursor.y + (region.y - boxSize.y) * 0.5f));

    // Draw image if texture is set
    auto texRef = (void*)(uintptr_t)(texture_->GetID());
    ImGui::Image(texRef, boxSize, ImVec2(0, 1), ImVec2(1, 0));

    ImGui::EndChild();

    // -----------------------------
    // Bottom: status / progress
    // -----------------------------
    ImGui::BeginChild("StatusBar", ImVec2(avail.x, bottomHeight), true);

    ImGui::TextUnformatted(statusMessage_.Get().c_str());
    ImGui::ProgressBar(progress_.Get(), ImVec2(-1.0f, 0.0f));

    ImGui::EndChild();

    ImGui::PopStyleVar();
    ImGui::End();
}
