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

option(USE_SYSTEM_LIBS "Use system-installed libraries instead of the bundled versions" $ENV{USE_SYSTEM_LIBS})

option(USE_SYSTEM_OPENAL "Use system-installed OpenAL-Soft instead of the bundled version" $ENV{USE_SYSTEM_OPENAL})
option(USE_SYSTEM_BULLET "Use system-installed Bullet3 instead of the bundled version" $ENV{USE_SYSTEM_BULLET})
option(USE_SYSTEM_GLFW "Use system-installed GLFW instead of the bundled version" $ENV{USE_SYSTEM_GLFW})
option(USE_SYSTEM_ASSIMP "Use system-installed Assimp instead of the bundled version" $ENV{USE_SYSTEM_ASSIMP})

if(USE_SYSTEM_LIBS)
    set(USE_SYSTEM_OPENAL ON CACHE BOOL "" FORCE)
    set(USE_SYSTEM_BULLET ON CACHE BOOL "" FORCE)
    set(USE_SYSTEM_GLFW ON CACHE BOOL "" FORCE)
    set(USE_SYSTEM_ASSIMP ON CACHE BOOL "" FORCE)
endif()

# ---- Vendor Libraries ----
# ---- GLFW ----
set(USE_VENDOR_GLFW ON CACHE BOOL "" FORCE)

if (USE_SYSTEM_GLFW)
    find_package(glfw3 QUIET)
    if(glfw3_FOUND)
        message(STATUS "Using system-installed GLFW: ${glfw3_VERSION}")
        set(USE_VENDOR_GLFW OFF CACHE BOOL "" FORCE)
    else()
        message(STATUS "Using bundled GLFW")
    endif()
endif()

if(USE_VENDOR_GLFW)
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
endif()


# ---- BULLET3 ----
set(USE_VENDOR_BULLET ON CACHE BOOL "" FORCE)
if (USE_SYSTEM_BULLET)
    find_package(Bullet QUIET)
    if(Bullet_FOUND)
        message(STATUS "Using system-installed Bullet: ${Bullet_VERSION}")
        set(USE_BUILD_BULLET OFF CACHE BOOL "" FORCE)
    else()
        message(STATUS "Using bundled Bullet3")
    endif()
endif()

if(USE_VENDOR_BULLET)
    set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
    set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "" FORCE)
    set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
    set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_BULLET3 ON CACHE BOOL "" FORCE)
    add_subdirectory(vendor/bullet3)
endif()

# --- OpenAL-Soft ----
set(USE_VENDOR_OPENAL ON CACHE BOOL "" FORCE)

if(USE_SYSTEM_OPENAL)
    find_package(OpenAL QUIET)
    if (OpenAL_FOUND)
        message(STATUS "Using system-installed OpenAL: ${OpenAL_VERSION}")
        set(USE_VENDOR_OPENAL OFF CACHE BOOL "" FORCE)
    else()
        message(STATUS "Using bundled OpenAL-Soft")
    endif()
endif()

if(USE_VENDOR_OPENAL)
    set(ALSOFT_ENABLE_MODULES OFF CACHE BOOL "" FORCE)
    set(LIBTYPE STATIC CACHE STRING "" FORCE)
    set(ALSOFT_EMBED_HRTF_DATA ON CACHE BOOL "" FORCE)
    set(ALSOFT_EAX ON CACHE BOOL "" FORCE)
    set(ALSOFT_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(ALSOFT_TESTS OFF CACHE BOOL "" FORCE)
    set(ALSOFT_UTILS OFF CACHE BOOL "" FORCE)
    set(ALSOFT_INSTALL_CONFIG OFF CACHE BOOL "" FORCE)
    set(ALSOFT_INSTALL OFF CACHE BOOL "" FORCE)
    if(WIN32)
        set(ALSOFT_BACKEND_WASAPI ON CACHE BOOL "" FORCE)
        set(ALSOFT_BACKEND_DSOUND OFF CACHE BOOL "" FORCE)
    endif()
    if (UNIX AND NOT APPLE)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(PIPEWIRE libpipewire-0.3>=0.3.23 QUIET)
        pkg_check_modules(PULSEAUDIO libpulse QUIET)
        pkg_check_modules(ALSA alsa QUIET)
        if(PIPEWIRE_FOUND)
            set(ALSOFT_BACKEND_PIPEWIRE ON CACHE BOOL "" FORCE)
        elseif(PULSEAUDIO_FOUND)
            set(ALSOFT_BACKEND_PULSEAUDIO ON CACHE BOOL "" FORCE)
        elseif(ALSA_FOUND)
            set(ALSOFT_BACKEND_ALSA ON CACHE BOOL "" FORCE)
        else()
            message(FATAL_ERROR "No suitable audio backend found for OpenAL-Soft. Please install PipeWire, PulseAudio, or ALSA development packages.")
        endif()
    endif()

    add_subdirectory(vendor/openal-soft)
endif()

# ---- Assimp ----
set(USE_VENDOR_ASSIMP ON CACHE BOOL "" FORCE)
if(USE_SYSTEM_ASSIMP)
    find_package(assimp QUIET)
    if(assimp_FOUND)
        message(STATUS "Using system-installed Assimp: ${assimp_VERSION}")
        set(USE_VENDOR_ASSIMP OFF CACHE BOOL "" FORCE)
    else()
        message(STATUS "Using bundled Assimp")
    endif()
endif()

if(USE_VENDOR_ASSIMP)
    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_MINIZIP ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
    add_subdirectory(vendor/assimp)
endif()

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
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/common/application.cpp

    src/common/asset-loader.cpp

    src/common/shader/shader.cpp

    src/common/mesh/mesh-utils.cpp

    src/common/model/model.cpp

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
    src/common/components/model-renderer.cpp
    src/common/components/free-camera-controller.cpp
    src/common/components/movement.cpp
    src/common/components/light.cpp

    src/game/components/collider.cpp
    src/game/components/enemy.cpp
    src/game/components/health.cpp
    src/game/components/player.cpp
    src/game/game-registration.cpp
    src/game/systems/collision-system.cpp
    src/game/systems/collision-debug-drawer.cpp

    src/common/systems/forward-renderer.cpp
    src/common/systems/audio-system.cpp

    src/common/audio/audio-utils.cpp
    src/common/components/audio-source.cpp
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        src/common
        src/game
        src/states
        vendor/bullet3/src
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
        assimp::assimp
)

if(USE_SYSTEM_OPENAL AND OpenAL_FOUND)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            OpenAL::OpenAL
    )
else()
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
        OpenAL
    )
endif()

if(USE_SYSTEM_ASSIMP AND assimp_FOUND)
    target_include_directories(${PROJECT_NAME}
        PRIVATE
            ${assimp_INCLUDE_DIRS}
    )
endif()

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        $<$<CONFIG:Debug>:COLLISION_DEBUG_DRAW>
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
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
