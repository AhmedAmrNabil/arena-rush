# Enter the Nix development environment
dev:
    nix develop

# Configure the project with CMake
configure:
    cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_COLOR_DIAGNOSTICS=ON \
        "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"

# Build the project using Ninja
build:
    #!/usr/bin/env bash
    if [[ ! -f build/build.ninja || CMakeLists.txt -nt build/build.ninja ]]; then
        echo "Configuring..."
        just configure
    fi
    cmake --build build -- -j$(nproc)

# Runs the game using the dedicated Nvidia/AMD GPU with optional config
[default]
run config="":
    #!/usr/bin/env bash
    if command -v nvidia-smi > /dev/null 2>&1; then
        export DRI_PRIME=1
        export __NV_PRIME_RENDER_OFFLOAD=1
        export __GLX_VENDOR_LIBRARY_NAME=nvidia
    elif lsmod | grep -i "amdgpu" > /dev/null 2>&1; then
        export DRI_PRIME=1
    fi
    just run-int {{ config }}

# Run the game/application on the integrated GPU for testing/sanity checks
run-int config="": build
    ./bin/ArenaRush {{ if config != "" { "-c " + config } else { "" } }}

# Package the project using Nix
package:
    nix build .

# Configure the WebAssembly/WebGL build with Emscripten
web-configure:
    #!/usr/bin/env bash
    if [[ -f build-web/build.ninja && ! CMakeLists.txt -nt build-web/build.ninja ]]; then
        exit 0
    fi
    if command -v emcmake >/dev/null 2>&1; then
        EMCMAKE=emcmake
    elif command -v emcmake.bat >/dev/null 2>&1; then
        EMCMAKE=emcmake.bat
    else
        printf '%s\n' "Emscripten not found. Run 'nix develop' with the updated flake, or install/activate emsdk so emcmake is on PATH."
        exit 127
    fi
    "$EMCMAKE" cmake -S . -B build-web -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_COLOR_DIAGNOSTICS=ON "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"

# Build the browser version into web-dist/
web-build: web-configure
    cmake --build build-web --parallel

# Serve the browser build locally
web-serve: web-build
    #!/usr/bin/env bash
    if command -v emrun >/dev/null 2>&1; then
        EMRUN=emrun
    elif command -v emrun.bat >/dev/null 2>&1; then
        EMRUN=emrun.bat
    else
        printf '%s\n' "Emscripten not found. Run 'nix develop' with the updated flake, or install/activate emsdk so emrun is on PATH."
        exit 127
    fi
    "$EMRUN" --no_browser --port 8080 web-dist/index.html

# Remove build artifacts
clean:
    rm -rf build build-web bin web-dist

# Create a desktop entry for the game
desktop:
    #!/usr/bin/env bash
    mkdir -p ~/.local/share/icons/hicolor/256x256/apps
    cp assets/textures/logo.png ~/.local/share/icons/hicolor/256x256/apps/arena-rush-dev.png

    mkdir -p ~/.local/share/applications
    sed \
        -e 's|@CMAKE_INSTALL_PREFIX@|{{ justfile_directory() }}|g' \
        -e 's|Icon=ArenaRush|Icon=arena-rush-dev|' \
        -e 's|Name=Arena Rush|Name=Arena Rush (dev)|' \
        ArenaRush.desktop.in > ~/.local/share/applications/arena-rush-dev.desktop

    update-desktop-database ~/.local/share/applications

clean-desktop:
    rm -f ~/.local/share/icons/hicolor/256x256/apps/arena-rush-dev.png
    rm -f ~/.local/share/applications/arena-rush-dev.desktop
