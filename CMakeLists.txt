cmake_minimum_required(VERSION 3.21)   # Selects the minimum version of CMake required to run this file
project(ArenaRush
    VERSION 0.1.0
    LANGUAGES C CXX
)

# ---- C++ Standard ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- Optional ccache ----
find_program(CCACHE_PROGRAM ccache)

if(CCACHE_PROGRAM)
    message(STATUS "ccache found: ${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
else()
    message(STATUS "ccache not found")
endif()

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

if(UNIX AND NOT APPLE AND NOT DEFINED GLFW_BUILD_WAYLAND)
    find_program(GLFW_WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner)
    find_package(PkgConfig QUIET)

    if(PkgConfig_FOUND)
        pkg_check_modules(GLFW_WAYLAND_DEPS QUIET
            wayland-client
            wayland-cursor
            wayland-egl
            xkbcommon
        )
    endif()

    if(GLFW_WAYLAND_SCANNER_EXECUTABLE AND GLFW_WAYLAND_DEPS_FOUND)
        set(GLFW_BUILD_WAYLAND ON CACHE BOOL "Build support for Wayland")
    else()
        set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "Build support for Wayland")

        if(NOT GLFW_WAYLAND_SCANNER_EXECUTABLE)
            set(_glfw_wayland_reason "missing wayland-scanner")
        elseif(NOT PkgConfig_FOUND)
            set(_glfw_wayland_reason "pkg-config is unavailable")
        else()
            set(_glfw_wayland_reason "missing Wayland development packages")
        endif()

        message(STATUS "Disabling GLFW Wayland (${_glfw_wayland_reason})")
    endif()
endif()

add_subdirectory(vendor/glfw)

# ---- GLAD ----
add_library(glad STATIC
    vendor/glad/src/gl.c
)
target_include_directories(glad
    PUBLIC vendor/glad/include
)

# ---- BULLET3 ----
set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET3 ON CACHE BOOL "" FORCE)
add_subdirectory(vendor/bullet3)

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
add_executable(${PROJECT_NAME}
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

    src/game/components/collider.cpp
    src/game/components/enemy.cpp
    src/game/components/health.cpp
    src/game/components/player.cpp
    src/game/game-registration.cpp

    src/common/systems/forward-renderer.cpp
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        src/common
        src/game
        src/states
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        glad
        glfw
        glm
        imgui
        utils
        BulletDynamics
        BulletCollision
        BulletSoftBody
        LinearMath
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/bin
)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

install(DIRECTORY assets/
    DESTINATION assets/
)

install(FILES config/app.jsonc
    DESTINATION config/
)

if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            OpenGL::GL
    )
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
endif()
