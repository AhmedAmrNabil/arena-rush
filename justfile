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
[default]
run config="": build
    ./bin/GAME_APPLICATION {{ if config != "" { "-c " + config } else { "" } }}

# Package the project using Nix
package:
    nix build ".?submodules=1"

# Remove build artifacts
clean:
    rm -rf build bin

# Setup imgcmp for Linux
[linux]
_setup-imgcmp:
    cp scripts/imgcmp-bin/imgcmp-linux scripts/imgcmp
    chmod +x scripts/imgcmp

# Setup imgcmp for Windows
[windows]
_setup-imgcmp:
    cp scripts/imgcmp-bin/imgcmp-win.exe scripts/imgcmp.exe

# Run tests for specified test cases
run-tests *TESTS: build _setup-imgcmp
    pwsh -File scripts/run-all.ps1 {{ TESTS }}

# Compare screenshots for tests
compare *TESTS: _setup-imgcmp
    pwsh -File scripts/compare-all.ps1 {{ TESTS }}

# Full pipeline: run tests then compare
test *TESTS: (run-tests TESTS) (compare TESTS)
