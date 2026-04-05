# Enter the Nix development environment
dev:
    nix develop

# Configure the project with CMake
configure:
    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_COLOR_DIAGNOSTICS=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5

# Build the project using Ninja
build: configure
    cmake --build build -- -j$(nproc)

# Run the game/application with optional config
# Runs the game using the dedicated Nvidia/AMD GPU
[default]
run config="": build
    DRI_PRIME=1 __NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia ./bin/GAME_APPLICATION {{ if config != "" { "-c " + config } else { "" } }}

# Run the game/application on the integrated GPU for testing/sanity checks
run-int config="": build
    ./bin/GAME_APPLICATION {{ if config != "" { "-c " + config } else { "" } }}

# Package the project using Nix
package:
    nix build ".?submodules=1"

# Remove build artifacts
clean:
    rm -rf build bin

# ------ Linux setup and test targets ------

# Setup imgcmp for Linux
[linux]
_setup-imgcmp:
    cp scripts/imgcmp-bin/imgcmp-linux scripts/imgcmp
    chmod +x scripts/imgcmp

# Run tests for specified test cases
[linux]
run-tests *TESTS: build _setup-imgcmp
    pwsh -File scripts/run-all.ps1 {{ TESTS }}

# Compare screenshots for tests
[linux]
compare *TESTS: _setup-imgcmp
    pwsh -File scripts/compare-all.ps1 {{ TESTS }}

# ------ Windows setup and test targets ------

# Setup imgcmp for Windows
[windows]
_setup-imgcmp:
    cp scripts/imgcmp-bin/imgcmp-win.exe scripts/imgcmp.exe

# Run tests for specified test cases
[windows]
run-tests *TESTS: build _setup-imgcmp
    powershell -ExecutionPolicy Bypass -File scripts/run-all.ps1 {{ TESTS }}

# Compare screenshots for tests
[windows]
compare *TESTS: _setup-imgcmp
    powershell -ExecutionPolicy Bypass -File scripts/compare-all.ps1 {{ TESTS }}

# Full pipeline: run tests then compare
test *TESTS: (run-tests TESTS) (compare TESTS)
