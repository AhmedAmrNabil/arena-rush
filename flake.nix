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
        pkgs = import nixpkgs { inherit system; };
      in
      {
        devShells.default = pkgs.mkShell {
          packages = with pkgs; [

            # build tools
            cmake
            ninja
            gcc
            pkg-config

            # libraries
            glfw
            glm
            libGL
            tinyobjloader

            # Display protocols
            # wayland
            wayland
            wayland-protocols
            libxkbcommon
            egl-wayland
            libdecor

            # X11
            xorg.libX11
            xorg.libXcursor
            xorg.libXrandr
            xorg.libXi
            xorg.libXinerama
          ];
        };
      }
    );
}
