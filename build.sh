#!/bin/bash
set -e

BUILD_TYPE="${1:-release}"

# === Fetch dependencies ===
mkdir -p "deps/gmod"
mkdir -p "deps/openvr/lib_linux64"

if [ ! -f "deps/gmod/Interface.h" ]; then
    wget -q -O deps/gmod/tmp.zip https://github.com/Facepunch/gmod-module-base/archive/15bf18f369a41ac3d4eba29ee0679f386ec628b7.zip
    unzip -j -o deps/gmod/tmp.zip gmod-module-base-15bf18f369a41ac3d4eba29ee0679f386ec628b7/include/GarrysMod/Lua/* -d deps/gmod/
    rm deps/gmod/tmp.zip
fi

if [ ! -f "deps/openvr/lib_linux64/libopenvr_api.so" ]; then
    wget -q -O deps/openvr/openvr.h https://github.com/ValveSoftware/openvr/raw/master/headers/openvr.h
    wget -q -O deps/openvr/lib_linux64/libopenvr_api.so https://github.com/ValveSoftware/openvr/raw/master/bin/linux64/libopenvr_api.so
fi

# === Build ===
if [ "$BUILD_TYPE" = "dev" ]; then
    mkdir -p build_dev
    cd build_dev
    cmake .. -DCMAKE_BUILD_TYPE=Debug -DVRMOD_DEV_BUILD=ON
    make -j"$(nproc)"
    echo ""
    echo "=== Running tests ==="
    ctest --output-on-failure
    cd ..
else
    mkdir -p build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release -DVRMOD_DEV_BUILD=OFF 2>&1
    make -j"$(nproc)" 2>&1
    cd ..
    mkdir -p install/GarrysMod/garrysmod/lua/bin
    cp build/gmcl_vrmod_linux64.dll install/GarrysMod/garrysmod/lua/bin/gmcl_vrmod_linux64.dll
fi
