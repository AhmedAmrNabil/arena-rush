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
if(EMSCRIPTEN AND CMAKE_HOST_WIN32)
    message(STATUS "ccache disabled for Emscripten on Windows")
    set(ASSIMP_BUILD_USE_CCACHE OFF CACHE BOOL "" FORCE)
else()
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        message(STATUS "ccache found: ${CCACHE_PROGRAM}")
        set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
        set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    else()
        message(STATUS "ccache not found")
    endif()
endif()

# ---- Linux Dependencies ----
if(UNIX AND NOT APPLE AND NOT EMSCRIPTEN)
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
if(EMSCRIPTEN)
    add_library(glfw INTERFACE)
else()
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
endif()


# ---- BULLET3 ----
set(USE_VENDOR_BULLET ON CACHE BOOL "" FORCE)
if (USE_SYSTEM_BULLET)
    find_package(Bullet QUIET)
    if(Bullet_FOUND)
        message(STATUS "Using system-installed Bullet: ${Bullet_VERSION}")
        set(USE_VENDOR_BULLET OFF CACHE BOOL "" FORCE)
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
if(EMSCRIPTEN)
    add_library(OpenAL INTERFACE)
else()
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
    if(EMSCRIPTEN)
        set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
        set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)
        set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
        set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
    endif()
    add_subdirectory(vendor/assimp)
    if(EMSCRIPTEN AND TARGET assimp)
        target_compile_definitions(assimp PRIVATE ASSIMP_BUILD_NO_GLTF1_IMPORTER)
        target_compile_options(assimp PRIVATE "-fexceptions")
    endif()
endif()

# ---- GLAD ----
if(EMSCRIPTEN)
    add_library(glad INTERFACE)
    target_include_directories(glad
        INTERFACE ${PROJECT_SOURCE_DIR}/web/include
    )
else()
    add_library(glad STATIC
        vendor/glad/src/gl.c
    )
    target_include_directories(glad
        PUBLIC vendor/glad/include
    )
endif()

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

target_compile_definitions(imgui
    PUBLIC
        GLFW_INCLUDE_NONE
)

if(EMSCRIPTEN)
    target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_ES3)
else()
    target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD2)
endif()

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

    src/common/post-process/bloom.cpp

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
    src/common/components/animation.cpp

    src/game/components/collider.cpp
    src/game/components/enemy.cpp
    src/game/components/health.cpp
    src/game/components/player.cpp
    src/game/components/projectile.cpp
    src/game/components/projectile-trail.cpp
    src/game/components/weapon.cpp
    src/game/game-registration.cpp
    src/game/systems/collision-system.cpp
    src/game/systems/collision-debug-drawer.cpp
    src/game/components/player-movement.cpp
    src/game/systems/player-movement-system.cpp
    src/game/systems/player-hud.cpp

    src/common/text/font.cpp
    src/common/text/text-renderer.cpp

    src/common/systems/forward-renderer.cpp
    src/common/systems/audio-system.cpp
    src/common/systems/animation-system.cpp

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

if(USE_SYSTEM_BULLET AND Bullet_FOUND)
    target_include_directories(${PROJECT_NAME}
        PRIVATE
            ${BULLET_INCLUDE_DIRS}
    )
endif()

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        $<$<CONFIG:Debug>:COLLISION_DEBUG_DRAW>
        $<$<AND:$<CONFIG:Release>,$<NOT:$<BOOL:${EMSCRIPTEN}>>>:WAYLAND_APPID=\"${PROJECT_NAME}\">
        GLFW_INCLUDE_NONE
)

if(EMSCRIPTEN)
    set(ARENA_WEB_ASSETS "${PROJECT_SOURCE_DIR}/assets@assets")
    set(ARENA_WEB_CONFIG "${PROJECT_SOURCE_DIR}/config@config")

    target_compile_options(${PROJECT_NAME}
        PRIVATE
            "-fexceptions"
    )

    target_link_options(${PROJECT_NAME}
        PRIVATE
            "SHELL:-fexceptions"
            "SHELL:-sNO_DISABLE_EXCEPTION_CATCHING=1"
            "SHELL:-sUSE_GLFW=3"
            "SHELL:-sMIN_WEBGL_VERSION=2"
            "SHELL:-sMAX_WEBGL_VERSION=2"
            "SHELL:-sFULL_ES3=1"
            "SHELL:-sALLOW_MEMORY_GROWTH=1"
            "SHELL:-sINITIAL_MEMORY=268435456"
            "SHELL:-sSTACK_SIZE=5242880"
            "SHELL:-sEXIT_RUNTIME=0"
            "SHELL:-sASSERTIONS=1"
            "SHELL:-sENVIRONMENT=web"
            "SHELL:--preload-file ${ARENA_WEB_ASSETS}"
            "SHELL:--preload-file ${ARENA_WEB_CONFIG}"
            "SHELL:--shell-file ${PROJECT_SOURCE_DIR}/web/shell.html"
    )

    set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/web-dist
        SUFFIX ".html"
    )
else()
    set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
    )
endif()

if(WIN32)
    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION .
    )
else()
    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
    )
endif()

install(DIRECTORY assets/
    DESTINATION assets
)

install(FILES config/app.jsonc
    DESTINATION config/
)

configure_file(
    ${CMAKE_SOURCE_DIR}/ArenaRush.desktop.in
    ${CMAKE_BINARY_DIR}/ArenaRush.desktop
    @ONLY
)

install(FILES ${CMAKE_BINARY_DIR}/ArenaRush.desktop
    DESTINATION share/applications
)

install(FILES assets/textures/logo.png
    DESTINATION share/icons/hicolor/256x256/apps
    RENAME ${PROJECT_NAME}.png
)

if(UNIX AND NOT APPLE AND NOT EMSCRIPTEN)
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

if(WIN32)
    target_sources(${PROJECT_NAME} PRIVATE ArenaRush.rc)
    target_link_libraries(${PROJECT_NAME} PRIVATE dwmapi)
endif()

# ---- CPack packaging ----
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Arena Rush - A simple OpenGL game built with C++ and CMake")
set(CPACK_PACKAGE_VENDOR "Arena Rush")
set(CPACK_PACKAGE_CONTACT "Arena Rush Maintainers")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(CPACK_GENERATOR "DEB;RPM")

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Arena Rush Maintainers")
set(CPACK_DEBIAN_PACKAGE_SECTION "games")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

set(CPACK_RPM_PACKAGE_GROUP "Amusements/Games")
set(CPACK_RPM_PACKAGE_LICENSE "Unknown")
set(CPACK_RPM_PACKAGE_AUTOREQPROV ON)

include(CPack)
