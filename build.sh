#!/bin/bash
set -e

mkdir -p "deps/gmod"
mkdir -p "deps/openvr/lib_linux64"

if [ ! -f "deps/gmod/Interface.h" ]; then
    wget -O deps/gmod/tmp.zip https://github.com/Facepunch/gmod-module-base/archive/15bf18f369a41ac3d4eba29ee0679f386ec628b7.zip
    unzip -j deps/gmod/tmp.zip gmod-module-base-15bf18f369a41ac3d4eba29ee0679f386ec628b7/include/GarrysMod/Lua/* -d deps/gmod/
    rm deps/gmod/tmp.zip
fi

if [ ! -f "deps/openvr/lib_linux64/libopenvr_api.so" ]; then
    wget -O deps/openvr/openvr.h https://github.com/ValveSoftware/openvr/raw/master/headers/openvr.h
    wget -O deps/openvr/lib_linux64/libopenvr_api.so https://github.com/ValveSoftware/openvr/raw/master/bin/linux64/libopenvr_api.so
fi

mkdir -p build_release
cd build_release
cmake .. -DVRMOD_DEV_BUILD=OFF
make -j$(nproc)
