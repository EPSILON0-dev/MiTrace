cmake_minimum_required(VERSION 3.16)

set(CMAKE_POLICY_VERSION_MINIMUM 3.16)

# Clear default flags
set(CMAKE_C_FLAGS_RELEASE "" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELWITHDEBINFO "" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_DEBUG "" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG "" CACHE STRING "" FORCE)

project(RayTracing)

set(ENABLE_PREVIEW_GUI ON CACHE BOOL "Enable Preview GUI")
set(ENABLE_BACKEND_BASIC ON CACHE BOOL "Enable Basic Backend")
set(TARGET_ARCH "native" CACHE STRING "Target architecture for optimization (e.g., native, x86-64, etc.)")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(FETCHCONTENT_QUIET OFF)
include(FetchContent)

# Nlohmann JSON library
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# Add spdlog library
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.17.0
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(spdlog)

# Add GLM library
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.3
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(glm)

# Add STB library
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(stb)

# Preview GUI dependencies
if (ENABLE_PREVIEW_GUI)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.3.9
		GIT_PROGRESS TRUE
    )
    
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(glfw)
    
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.90.4
		GIT_PROGRESS TRUE
    )
    
    FetchContent_MakeAvailable(imgui)
    
    set(IMGUI_BACKEND_FILES
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    )

    set (PREVIEW_SRC_FILES)
endif()

# Basic Backend
if (ENABLE_BACKEND_BASIC)
    set (BACKEND_BASIC_SRC_FILES
        src/Tracer/Backends/Basic/Intersect.cpp
        src/Tracer/Backends/Basic/RayHit.cpp
        src/Tracer/Backends/Basic/Tracer.cpp
    )
endif()

# Source Files
set(SRC_FILES
    src/main.cpp
    src/Loader/GLTF.cpp
    src/Loader/Config.cpp
    src/Preview/Preview.cpp 
    src/Scene/BVH.cpp
    src/Scene/Image.cpp
    src/Scene/Mesh.cpp
    src/Scene/Scene.cpp
    src/Common/STB_Impl.cpp
    ${BACKEND_BASIC_SRC_FILES}
    ${IMGUI_BACKEND_FILES}
)

add_executable(RayTracing ${SRC_FILES})

target_link_libraries(RayTracing PRIVATE spdlog)
target_include_directories(RayTracing PRIVATE src)

target_include_directories(RayTracing PRIVATE ${nlohmann_json_SOURCE_DIR}/include)
target_include_directories(RayTracing PRIVATE ${spdlog_SOURCE_DIR})
target_include_directories(RayTracing PRIVATE ${glm_SOURCE_DIR})
target_include_directories(RayTracing PRIVATE ${stb_SOURCE_DIR})

if (ENABLE_BACKEND_BASIC)
    target_compile_definitions(RayTracing PRIVATE ENABLE_BACKEND_BASIC=1)
endif()

if (ENABLE_PREVIEW_GUI)
    target_compile_definitions(RayTracing PRIVATE ENABLE_PREVIEW_GUI=1)
    target_include_directories(RayTracing PRIVATE ${imgui_SOURCE_DIR}/backends)
    target_link_libraries(RayTracing PRIVATE imgui glfw GL)
endif()

target_compile_options(RayTracing PRIVATE
    $<$<CONFIG:Release>:-Ofast -march=${TARGET_ARCH}>
    $<$<CONFIG:RelWithDebInfo>:-Ofast -g -march=${TARGET_ARCH}>
    $<$<CONFIG:Debug>:-O0 -g -fsanitize=address,undefined>
    -pthread -Wall -Wextra -Wpedantic -Werror
)

target_link_options(RayTracing PRIVATE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)
