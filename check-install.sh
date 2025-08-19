#!/bin/sh

: ${CORES:=$(nproc 2>/dev/null)}
: ${CORES:=$(sysctl -n hw.ncpu 2>/dev/null)}
: ${CORES:=4}

if command -v ninja >/dev/null; then
    CMAKE_GENERATOR="Ninja"
    BUILDCMD=ninja
else
    case $(uname) in
    MINGW*)
        CMAKE_GENERATOR="MSYS Makefiles"
        ;;
    esac
    BUILDCMD=make
    BUILDCMD_CORES=$CORES
fi

if [ "$1" = "mingw" ]; then
    SUFFIX=-mingw
    : ${TRIPLE:=x86_64-w64-mingw32}
    CONFIG_FLAGS="--host=$TRIPLE"
    CMAKE_FLAGS="-DCMAKE_SYSTEM_NAME=Windows -DCMAKE_C_COMPILER=$TRIPLE-gcc -DCMAKE_CXX_COMPILER=$TRIPLE-g++"
    MINGW=1
fi

set -ex

cd $(dirname $0)

./autogen.sh

[ -z "$CLEAN" ] || rm -rf build-check-autotools$SUFFIX
mkdir -p build-check-autotools$SUFFIX
cd build-check-autotools$SUFFIX

../configure --prefix=$(pwd)/../install-autotools$SUFFIX $CONFIG_FLAGS
make -j$CORES
rm -rf ../install-autotools$SUFFIX
make -j$CORES install

cd ..
[ -z "$CLEAN" ] || rm -rf build-check-cmake$SUFFIX
mkdir -p build-check-cmake$SUFFIX
cd build-check-cmake$SUFFIX

rm -rf CMakeCache.txt
cmake \
    ${CMAKE_GENERATOR+-G} "$CMAKE_GENERATOR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$(pwd)/../install-cmake$SUFFIX \
    -DBUILD_SHARED_LIBS=ON \
    $CMAKE_FLAGS \
    ..

$BUILDCMD ${BUILDCMD_CORES:+-j${BUILDCMD_CORES}}
rm -rf ../install-cmake$SUFFIX
$BUILDCMD install

cmake -DBUILD_SHARED_LIBS=OFF ..

$BUILDCMD ${BUILDCMD_CORES:+-j${BUILDCMD_CORES}}
$BUILDCMD install

cd ..

cd install-autotools$SUFFIX
find . | sed 's/^..//' | sort | grep -v '\.la$' > ../listing-autotools.txt
cd ../install-cmake$SUFFIX
find . | sed 's/^..//' | sort | grep -v '^lib/cmake' > ../listing-cmake.txt
cd ..

if [ "$(uname)" = "Darwin" ] && [ -z "$MINGW" ]; then
    # On macOS, cmake installs both libfdk-aac.dylib, libfdk-aac.2.dylib and
    # libfdk-aac.2.0.3.dylib (with the first two being symlinks to the latter),
    # while autotools only installs libfdk-aac.dylib and libfdk-aac.2.dylib.
    cat listing-cmake.txt | grep -v 'lib.*\.\d\.\d\.\d\.dylib' > tmp
    mv tmp listing-cmake.txt
fi

diff -u listing-autotools.txt listing-cmake.txt

# Check the soname, or equivalent.
if [ -n "$MINGW" ]; then
    get_dllname() {
        $TRIPLE-dlltool --identify $1
    }
    SONAME_AUTOTOOLS=$(get_dllname install-autotools-mingw/lib/libfdk-aac.dll.a)
    SONAME_CMAKE=$(get_dllname install-cmake-mingw/lib/libfdk-aac.dll.a)
elif [ "$(uname)" = "Darwin" ]; then
    get_soname() {
        basename $(otool -D $1 | tail -1)
    }
    SONAME_AUTOTOOLS=$(get_soname install-autotools/lib/libfdk-aac.dylib)
    SONAME_CMAKE=$(get_soname install-cmake/lib/libfdk-aac.dylib)
else
    get_soname() {
        readelf -d $1 | grep SONAME | sed -e 's/.*soname: //' -e 's/^\[//' -e 's/\]$//'
    }
    SONAME_AUTOTOOLS=$(get_soname install-autotools/lib/libfdk-aac.so)
    SONAME_CMAKE=$(get_soname install-cmake/lib/libfdk-aac.so)
fi

if [ "$SONAME_AUTOTOOLS" != "$SONAME_CMAKE" ]; then
    echo Autotools SONAME $SONAME_AUTOTOOLS differs from CMake $SONAME_CMAKE
    exit 1
fi
