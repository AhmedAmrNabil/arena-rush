{
  description = "Arena Rush - A simple OpenGL game built with C++ and CMake";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    self.submodules = true;
  };

  outputs =
    {
      self,
      nixpkgs,
      flake-utils,
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        packageName = "ArenaRush";
        pkgs = import nixpkgs { inherit system; };
        buildInputs = with pkgs; [
          # libraries
          libGL
          libffi

          # Audio
          pipewire

          # Wayland
          wayland
          wayland-protocols
          libxkbcommon
          egl-wayland
          libdecor
          wayland-scanner
          extra-cmake-modules # for wayland

          # X11
          libx11
          libxcursor
          libxrandr
          libxi
          libxinerama
        ];

        nativeBuildInputs = with pkgs; [
          cmake
          ninja
          pkg-config
          makeWrapper
        ];

        LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath buildInputs;
      in
      {
        devShells.default = pkgs.mkShell {

          inherit buildInputs nativeBuildInputs LD_LIBRARY_PATH;

          packages = with pkgs; [
            # helper
            just
            powershell
            glslang
            just-lsp
            ccache
            clang-tools
          ];
        };

        packages = {
          default = self.packages.${system}.opengl_app;
          opengl_app = pkgs.stdenv.mkDerivation {
            pname = packageName;
            version = "0.1.0";

            src = self;

            inherit buildInputs nativeBuildInputs;

            cmakeFlags = [
              "-DCMAKE_BUILD_TYPE=Release"
              "-DGLFW_BUILD_WAYLAND=1"
              "-DGLFW_BUILD_X11=1"
              "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
            ];

            installPhase = ''
              cmake --install . --prefix $out
              wrapProgram $out/bin/ArenaRush \
                --chdir $out \
                --suffix LD_LIBRARY_PATH : ${LD_LIBRARY_PATH}
            '';
          };
        };
      }
    );
}
