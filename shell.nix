{
  pkgs ? import <nixpkgs> { },
}:
let
  arenarush = pkgs.callPackage ./package.nix { };
in
pkgs.mkShell {
  name = "arena-rush";

  inputsFrom = [ arenarush ];

  env.USE_SYSTEM_LIBS = "ON";
  env.LD_LIBRARY_PATH = "${pkgs.addDriverRunpath.driverLink}/lib";

  packages = with pkgs; [
    just
    powershell
    glslang
    # emscripten
    just-lsp
    ccache
    clang-tools
    gdb
    desktop-file-utils
  ];
}
