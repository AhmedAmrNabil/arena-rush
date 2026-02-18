cmake_minimum_required(VERSION 3.21)

project(opengl_project
    VERSION 1.0.0
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- Dependencies ----
find_package(OpenGL REQUIRED)

# ---- TinyObjLoader (vendored) ----
add_library(tinyobjloader INTERFACE)
target_include_directories(tinyobjloader
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/tinyobjloader
)

# ---- GLM (vendored) ----
add_library(glm INTERFACE)
target_include_directories(glm
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glm
)

# ---- GLFW (vendored) ----
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/glfw)

# ---- GLAD (vendored) ----
add_library(glad STATIC
    vendor/glad/src/glad.c
)
target_include_directories(glad
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glad/include
)

# ---- IMGUI (vendored) ----
add_library(imgui STATIC
    vendor/imgui/imgui_demo.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/backends/imgui_impl_glfw.cpp
    vendor/imgui/backends/imgui_impl_opengl3.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_widgets.cpp
    vendor/imgui/imgui.cpp
)
target_include_directories(imgui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends
)
target_link_libraries(imgui
    PRIVATE
        glfw
        OpenGL::GL
)

# ---- STB Image (vendored) ----
add_library(stb_image STATIC
    vendor/stb/src/stb_image.cpp
)
target_include_directories(stb_image
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb/include
)

# ---- Executable ----
add_executable(opengl_app
    src/main.cpp
)

target_include_directories(opengl_app
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(opengl_app
    PRIVATE
        glad
        OpenGL::GL
        glfw
        glm
        tinyobjloader
        imgui
        stb_image
)

install(TARGETS opengl_app
    RUNTIME DESTINATION bin
)


# ---- Warnings ----
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(opengl_app PRIVATE -Wall -Wextra -Wpedantic)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(opengl_app PRIVATE /W4 /permissive-)
endif()
