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
set(COMMON_SOURCES
        source/common/application.hpp
        source/common/application.cpp
        source/common/input/keyboard.hpp
        source/common/input/mouse.hpp

        source/common/asset-loader.cpp
        source/common/asset-loader.hpp
        source/common/deserialize-utils.hpp

        source/common/shader/shader.hpp
        source/common/shader/shader.cpp

        source/common/mesh/vertex.hpp
        source/common/mesh/mesh.hpp
        source/common/mesh/mesh-utils.hpp
        source/common/mesh/mesh-utils.cpp

        source/common/texture/sampler.hpp
        source/common/texture/sampler.cpp
        source/common/texture/texture2d.hpp
        source/common/texture/texture-utils.hpp
        source/common/texture/texture-utils.cpp
        source/common/texture/screenshot.hpp
        source/common/texture/screenshot.cpp

        source/common/material/pipeline-state.hpp
        source/common/material/pipeline-state.cpp
        source/common/material/material.hpp
        source/common/material/material.cpp

        source/common/ecs/component.hpp
        source/common/ecs/transform.hpp
        source/common/ecs/transform.cpp
        source/common/ecs/entity.hpp
        source/common/ecs/entity.cpp
        source/common/ecs/world.hpp
        source/common/ecs/world.cpp

        source/common/components/camera.hpp
        source/common/components/camera.cpp
        source/common/components/mesh-renderer.hpp
        source/common/components/mesh-renderer.cpp
        source/common/components/free-camera-controller.hpp
        source/common/components/free-camera-controller.cpp
        source/common/components/movement.hpp
        source/common/components/movement.cpp
        source/common/components/component-deserializer.hpp

        source/common/systems/forward-renderer.hpp
        source/common/systems/forward-renderer.cpp
        source/common/systems/free-camera-controller.hpp
        source/common/systems/movement.hpp
)

set(STATES_SOURCES
        source/states/play-state.hpp
        source/states/menu-state.hpp
        source/states/shader-test-state.hpp
        source/states/mesh-test-state.hpp
        source/states/transform-test-state.hpp
        source/states/pipeline-test-state.hpp
        source/states/texture-test-state.hpp
        source/states/sampler-test-state.hpp
        source/states/material-test-state.hpp
        source/states/entity-test-state.hpp
        source/states/renderer-test-state.hpp
)

add_executable(GAME_APPLICATION
    source/main.cpp
    ${COMMON_SOURCES}
    ${STATES_SOURCES}
)

target_include_directories(GAME_APPLICATION
    PRIVATE
        source/common
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

if(UNIX AND NOT APPLE)
    target_link_libraries(GAME_APPLICATION
        PRIVATE
            OpenGL::GL
    )
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(GAME_APPLICATION PRIVATE -Wall -Wextra -Wpedantic)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(GAME_APPLICATION PRIVATE /W4 /permissive-)
endif()