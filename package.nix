{
  lib,
  stdenv,
  cmake,
  ninja,
  pkg-config,
  makeWrapper,
  addDriverRunpath,
  libGL,
  fetchFromGitHub,
  pipewire,
  openal-soft,
  bullet,
  glfw,
  assimp,
  zlib,
  # Wayland
  wayland,
  wayland-protocols,
  libxkbcommon,
  egl-wayland,
  libdecor,
  wayland-scanner,
  extra-cmake-modules,
  # X11
  libx11,
  libxcursor,
  libxrandr,
  libxi,
  libxinerama,
  withWayland ? true,
  withX11 ? true,
}:

assert lib.assertMsg (
  withWayland || withX11
) "arena-rush: need at least one of withWayland or withX11";

stdenv.mkDerivation {
  pname = "arena-rush";
  version = "0.1.0";

  src = fetchFromGitHub {
    owner = "AhmedAmrNabil";
    repo = "arena-rush";
    rev = "55ae0beecaa689b8001585c41fd14f4fa833f970";
    hash = "sha256-J8fRAbVfuyRw4sflpGdBjBQU9tQktelkz+besq24meo=";
  };

  nativeBuildInputs = [
    cmake
    ninja
    pkg-config
    makeWrapper
    addDriverRunpath
  ]
  ++ lib.optionals withWayland [
    wayland-scanner
    extra-cmake-modules
  ];

  buildInputs = [
    libGL
    pipewire
    openal-soft
    bullet
    glfw
    assimp
    zlib
  ]
  ++ lib.optionals withWayland [
    wayland
    wayland-protocols
    libxkbcommon
    egl-wayland
    libdecor
  ]
  ++ lib.optionals withX11 [
    libx11
    libxcursor
    libxrandr
    libxi
    libxinerama
  ];

  cmakeFlags = [
    "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"
    "-DUSE_SYSTEM_LIBS=ON"
  ];

  postFixup = ''
    wrapProgram "$out/bin/ArenaRush" \
      --chdir "$out"
  '';

  meta = {
    description = "A simple OpenGL FPS game built with C++ and CMake";
    mainProgram = "ArenaRush";
    platforms = lib.platforms.linux;
  };
}
