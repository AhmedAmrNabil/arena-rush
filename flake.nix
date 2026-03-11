{
  description = "Modern OpenGL (GLAD + GLFW + GLM)";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.11";
    flake-utils.url = "github:numtide/flake-utils";
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
        packageName = "project-opengl-app";
        pkgs = import nixpkgs { inherit system; };
        buildInputs = with pkgs; [
          # libraries
          libGL
          libffi

          # Wayland
          wayland
          wayland-protocols
          libxkbcommon
          egl-wayland
          libdecor
          wayland-scanner
          extra-cmake-modules # for wayland

          # X11
          xorg.libX11
          xorg.libXcursor
          xorg.libXrandr
          xorg.libXi
          xorg.libXinerama
        ];

        nativeBuildInputs = with pkgs; [
          cmake
          ninja
          pkg-config
          makeWrapper
        ];
      in
      {
        devShells.default = pkgs.mkShell {

          inherit buildInputs nativeBuildInputs;

          packages = with pkgs; [
            # helper
            just
            powershell
            glslang
            just-lsp
          ];

          LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath buildInputs;
        };

        packages = {
          default = self.packages.${system}.opengl_app;
          opengl_app = pkgs.stdenv.mkDerivation {
            pname = packageName;
            version = "1.0.0";

            src = self;

            inherit buildInputs nativeBuildInputs;

            cmakeFlags = [
              "-DCMAKE_BUILD_TYPE=Release"
              "-DGLFW_BUILD_WAYLAND=1"
              "-DGLFW_BUILD_X11=1"
            ];

            installPhase = ''
              cmake --install . --prefix $out
              wrapProgram $out/bin/GAME_APPLICATION \
                --set LD_LIBRARY_PATH ${pkgs.libGL}/lib:${pkgs.wayland}/lib:${pkgs.libxkbcommon}/lib:$LD_LIBRARY_PATH
            '';
          };
        };
      }
    );
}
