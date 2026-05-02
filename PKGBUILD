pkgname=arenarush
_pkgname=ArenaRush
pkgver=0.1.0
pkgrel=1
pkgdesc="Arena Rush - A simple OpenGL game built with C++ and CMake"
arch=('x86_64')
url="https://github.com/AhmedAmrNabil/arena-rush"
license=('unknown')

depends=(
    libglvnd
    libffi
    pipewire
    openal
    bullet
    glfw
    assimp
    zlib
    wayland
    wayland-protocols
    libxkbcommon
    egl-wayland
    libdecor
    libx11
    libxcursor
    libxrandr
    libxi
    libxinerama
)

makedepends=(
    cmake
    ninja
    pkgconf
    git
    extra-cmake-modules
)

source=("git+$url.git")
sha256sums=('SKIP')

build() {
    cmake -S "$srcdir/arena-rush" -B "$srcdir/build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DGLFW_BUILD_WAYLAND=1 \
    -DGLFW_BUILD_X11=1 \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
    -DUSE_SYSTEM_LIBS=ON

    cmake --build "$srcdir/build"
}

package() {
    DESTDIR="$pkgdir" cmake --install "$srcdir/build"
}
