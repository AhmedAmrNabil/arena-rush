dev:
	nix develop

configure:
	cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug  -DCMAKE_COLOR_DIAGNOSTICS=ON

build: configure
	cmake --build build

run: build
	cd build && ./opengl_app

package:
	nix build ".?submodules=1"

clean:
	rm -rf build
