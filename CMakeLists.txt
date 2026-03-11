cmake_minimum_required(VERSION 3.21)   # Selects the minimum version of CMake required to run this file
project(GFX_LAB
    VERSION 0.1.0
    LANGUAGES C CXX
)

# ---- C++ Standard ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Linux Dependencies ----
if(UNIX AND NOT APPLE)
    find_package(OpenGL REQUIRED)
endif()

# ---- Vendor Libraries ----
# ---- GLFW ----
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_USE_HYBRID_HPG ON CACHE BOOL "" FORCE)

add_subdirectory(vendor/glfw)

# ---- GLAD ----
add_library(glad STATIC
    vendor/glad/src/gl.c
)
target_include_directories(glad
    PUBLIC vendor/glad/include
)

# ---- GLM (Header-only) ----
add_library(glm INTERFACE)
target_include_directories(glm
    INTERFACE vendor/glm
)

# ---- Utils (Header-only) ----
add_library(utils INTERFACE)
target_include_directories(utils
    INTERFACE vendor/utils
)

# ---- ImGui ----
add_library(imgui STATIC
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_demo.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/imgui_impl/imgui_impl_glfw.cpp
    vendor/imgui/imgui_impl/imgui_impl_opengl3.cpp
)

target_include_directories(imgui
    PUBLIC
        vendor/imgui
        vendor/imgui/imgui_impl
)

target_link_libraries(imgui
    PUBLIC 
        glfw
        glad
)


# ---- Application Sources ----
add_executable(GAME_APPLICATION
    src/main.cpp
    src/common/application.cpp

    src/common/asset-loader.cpp

    src/common/shader/shader.cpp

    src/common/mesh/mesh-utils.cpp

    src/common/texture/sampler.cpp
    src/common/texture/texture-utils.cpp
    src/common/texture/screenshot.cpp

    src/common/material/pipeline-state.cpp
    src/common/material/material.cpp

    src/common/ecs/transform.cpp
    src/common/ecs/entity.cpp
    src/common/ecs/world.cpp

    src/common/components/camera.cpp
    src/common/components/mesh-renderer.cpp
    src/common/components/free-camera-controller.cpp
    src/common/components/movement.cpp

    src/common/systems/forward-renderer.cpp
)

target_include_directories(GAME_APPLICATION
    PRIVATE
        src/common
        src/states
)

target_link_libraries(GAME_APPLICATION
    PRIVATE
        glad
        glfw
        glm
        imgui
        utils
)

set_target_properties(GAME_APPLICATION PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/bin
)

install(TARGETS GAME_APPLICATION
    RUNTIME DESTINATION bin
)

install(DIRECTORY assets/
    DESTINATION bin/assets
)



if(UNIX AND NOT APPLE)
    target_link_libraries(GAME_APPLICATION
        PRIVATE
            OpenGL::GL
    )
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(GAME_APPLICATION PRIVATE -Wall)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(GAME_APPLICATION PRIVATE /W4)
endif()