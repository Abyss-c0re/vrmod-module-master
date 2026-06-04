#!/bin/bash
set -e

mkdir -p "deps/gmod"
mkdir -p "deps/openvr/lib_linux64"
mkdir -p "deps/openxr"

if [ ! -f "deps/gmod/Interface.h" ]; then
    wget -O deps/gmod/tmp.zip https://github.com/Facepunch/gmod-module-base/archive/15bf18f369a41ac3d4eba29ee0679f386ec628b7.zip
    unzip -j deps/gmod/tmp.zip gmod-module-base-15bf18f369a41ac3d4eba29ee0679f386ec628b7/include/GarrysMod/Lua/* -d deps/gmod/
    rm deps/gmod/tmp.zip
fi

if [ ! -f "deps/openvr/lib_linux64/libopenvr_api.so" ]; then
    wget -O deps/openvr/openvr.h https://github.com/ValveSoftware/openvr/raw/master/headers/openvr.h
    wget -O deps/openvr/lib_linux64/libopenvr_api.so https://github.com/ValveSoftware/openvr/raw/master/bin/linux64/libopenvr_api.so
fi

# Download OpenXR headers (from official Khronos release)
# We only need the headers; the loader is provided at runtime by the OpenXR runtime (Monado, SteamVR, etc.).
# Using GitHub's archive tarball (always available for tags) + precise extraction.
if [ ! -f "deps/openxr/openxr/openxr.h" ]; then
    XR_TAG="release-1.1.60"
    echo "Downloading OpenXR-SDK ${XR_TAG} headers..."
    TMP_TAR="/tmp/openxr-sdk-${XR_TAG}.tar.gz"
    wget -O "$TMP_TAR" "https://github.com/KhronosGroup/OpenXR-SDK/archive/refs/tags/${XR_TAG}.tar.gz"
    tar --wildcards -xzf "$TMP_TAR" --strip-components=2 -C deps/openxr "OpenXR-SDK-${XR_TAG}/include/openxr"
    rm -f "$TMP_TAR"
    echo "OpenXR headers installed to deps/openxr/openxr/"
fi

mkdir -p build_release
cd build_release
cmake .. -DCMAKE_BUILD_TYPE=Release -DVRMOD_BUILD_TESTS=OFF
make -j$(nproc) vrmod_release
