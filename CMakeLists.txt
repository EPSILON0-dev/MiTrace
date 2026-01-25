# Set the minimum required version for compatibility with dependencies
cmake_minimum_required(VERSION 3.16)

set(CMAKE_POLICY_VERSION_MINIMUM 3.16)

project(RayTracing)

# set(TARGET_ARCH "haswell")  # Temporary fix for callgrind compatibility
set(TARGET_ARCH "native")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build type configurations
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

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

# Add nlohmann/json library
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)


# Ensure imgui and its backends are linked
set(IMGUI_BACKEND_FILES
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# Add ImGui backend files to the build
set(SRC_FILES
    src/main.cpp
    src/GUI/GUI.cpp
    src/GUI/GL_Texture.cpp
    src/Loader/GLTF_Loader.cpp
    src/Trace/Camera.cpp
    src/Trace/TextureImage.cpp
    src/Trace/Mesh.cpp
    src/Trace/MeshInstance.cpp
    src/Trace/Intersect.cpp
    src/Trace/RayHit.cpp
    src/Trace/Trace.cpp
    src/Config/Config.cpp
    src/Common/STB_Impl.cpp
    ${IMGUI_BACKEND_FILES}
)

# Force STB implementation to not include PCH
set_source_files_properties(src/Common/STB_Impl.cpp PROPERTIES SKIP_PRECOMPILE_HEADERS ON)

# Main executable
add_executable(RayTracing ${SRC_FILES})

# Link imgui library
target_link_libraries(RayTracing PUBLIC imgui glfw GL spdlog fmt)

# Include directories
target_include_directories(RayTracing PRIVATE src)
target_include_directories(RayTracing PRIVATE ${imgui_SOURCE_DIR}/backends)

# Compiler options
# Arch temporarily set to x86-64 for callgrind compatibility
target_compile_options(RayTracing PRIVATE
    $<$<CONFIG:Release>:-Ofast -march=${TARGET_ARCH}>
    $<$<CONFIG:RelWithDebInfo>:-Ofast -g -march=${TARGET_ARCH}>
    $<$<CONFIG:Debug>:-O0 -g -fsanitize=address,undefined>
    -pthread -Wall -Wextra -Wpedantic
)

# Link options
target_link_options(RayTracing PRIVATE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)
