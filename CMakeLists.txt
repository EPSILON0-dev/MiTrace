cmake_minimum_required(VERSION 3.16)

project(RayTracing)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type configurations
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -fsanitize=address,undefined")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -march=native")

# Include directories
include_directories(src)

# Fetch glfw and imgui globally
include(FetchContent)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.9
)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.4
)

FetchContent_MakeAvailable(imgui)

# Ensure imgui backends are included
include_directories(${imgui_SOURCE_DIR}/backends)

# Ensure imgui and its backends are linked
set(IMGUI_BACKEND_FILES
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# Add ImGui backend files to the build
set(SRC_FILES
    src/main.cpp
    src/gui/gui.cpp
    src/gui/gl_texture.cpp
    src/trace/camera.cpp
    src/trace/trace.cpp
    ${IMGUI_BACKEND_FILES}
)

# Main executable
add_executable(RayTracing ${SRC_FILES})

# Link imgui library
target_link_libraries(RayTracing PUBLIC imgui glfw GL)
