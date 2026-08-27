{
  description = "Arena Rush - A simple OpenGL game built with C++ and CMake";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
  };

  outputs =
    inputs@{
      nixpkgs,
      flake-parts,
      ...
    }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = nixpkgs.lib.systems.flakeExposed;
      perSystem =
        {
          pkgs,
          lib,
          self',
          ...
        }:
        let
          arena-rush = pkgs.callPackage ./package.nix { };
        in
        {
          devShells.default = import ./shell.nix { inherit pkgs; };

          packages = {
            inherit arena-rush;
            default = self'.packages.arena-rush;
          };

          apps = {
            arena-rush = {
              program = lib.getExe arena-rush;
              inherit (arena-rush) meta;
            };
            default = self'.apps.arena-rush;
          };
        };
    };
}
