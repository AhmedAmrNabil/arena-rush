dev:
	nix develop

configure:
	cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug

build: configure
	cmake --build build

run: build
	./build/opengl_app

clean:
	rm -rf build
