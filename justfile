dev:
	nix develop

configure:
	cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_COLOR_DIAGNOSTICS=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5

build: configure
	cmake --build build -- -j$(nproc)

run config="": build
	./bin/GAME_APPLICATION {{ if config != "" { "-c " + config } else { "" } }}

package:
	nix build

clean:
	rm -rf build bin

# --- OS-specific imgcmp setup
[linux]
_setup-imgcmp:
	cp scripts/imgcmp-bin/imgcmp-linux scripts/imgcmp
	chmod +x scripts/imgcmp

[windows]
_setup-imgcmp:
	cp scripts/imgcmp-bin/imgcmp-win.exe scripts/imgcmp.exe

run-tests *TESTS: build _setup-imgcmp
	pwsh -File scripts/run-all.ps1 {{TESTS}}

# --- Compare screenshots (or a subset)

compare *TESTS: _setup-imgcmp
	pwsh -File scripts/compare-all.ps1 {{TESTS}}

# --- Full pipeline: run tests then compare

test *TESTS: (run-tests TESTS) (compare TESTS)
