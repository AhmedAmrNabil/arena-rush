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

          # Wayland
          wayland
          wayland-protocols
          libxkbcommon
          egl-wayland
          libdecor
          wayland-scanner

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
          ];

          shellHook = "export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${pkgs.wayland}/lib:${pkgs.libxkbcommon}/lib";
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
            ];

            installPhase = ''
              mkdir -p $out/bin
              cp ./opengl_app $out/bin/
              wrapProgram $out/bin/opengl_app \
                --set LD_LIBRARY_PATH ${pkgs.libGL}/lib:${pkgs.wayland}/lib:${pkgs.libxkbcommon}/lib:$LD_LIBRARY_PATH
            '';
          };
        };
      }
    );
}
