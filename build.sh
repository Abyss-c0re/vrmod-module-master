#!/bin/bash
set -e
ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

INSTALL_GM="$ROOT/install/GarrysMod"
MODULE_DIR="$INSTALL_GM/garrysmod/lua/bin"
BIN_DIR="$INSTALL_GM/bin"
BIN_LINUX64="$BIN_DIR/linux64"

mkdir -p "deps/gmod"
mkdir -p "deps/openvr/lib_linux64"
mkdir -p "$MODULE_DIR" "$BIN_LINUX64"

if [ ! -f "deps/gmod/Interface.h" ]; then
    wget -O deps/gmod/tmp.zip https://github.com/Facepunch/gmod-module-base/archive/15bf18f369a41ac3d4eba29ee0679f386ec628b7.zip
    unzip -j deps/gmod/tmp.zip gmod-module-base-15bf18f369a41ac3d4eba29ee0679f386ec628b7/include/GarrysMod/Lua/* -d deps/gmod/
    rm deps/gmod/tmp.zip
fi

if [ ! -f "deps/openvr/openvr.h" ]; then
    wget -O deps/openvr/openvr.h https://github.com/ValveSoftware/openvr/raw/v2.5.1/headers/openvr.h
fi

# Prefer installed SteamVR lib (matches runtime; avoids Interface Not Found 105)
STEAMVR_LIB="${HOME}/.local/share/Steam/steamapps/common/SteamVR/bin/linux64/libopenvr_api.so"
if [ -f "$STEAMVR_LIB" ]; then
  cp -f "$STEAMVR_LIB" deps/openvr/lib_linux64/libopenvr_api.so
elif [ ! -f "deps/openvr/lib_linux64/libopenvr_api.so" ]; then
  wget -O deps/openvr/lib_linux64/libopenvr_api.so \
    https://github.com/ValveSoftware/openvr/raw/v2.5.1/bin/linux64/libopenvr_api.so
fi

# Package extras (match modules.zip layout)
if [ ! -f "deps/openvr/openvr_api.dll" ]; then
  wget -O deps/openvr/openvr_api.dll \
    https://github.com/ValveSoftware/openvr/raw/v2.5.1/bin/win32/openvr_api.dll
fi
if [ ! -f "deps/openvr/openvr_license" ]; then
  wget -O deps/openvr/openvr_license \
    https://raw.githubusercontent.com/ValveSoftware/openvr/v2.5.1/LICENSE
fi

mkdir -p build_release
cd build_release
cmake .. -DCMAKE_BUILD_TYPE=Release -DVRMOD_BUILD_TESTS=OFF
make -j$(nproc) vrmod_release
cd "$ROOT"

# Canonical install layout (matches Desktop modules.zip reference):
#   install/GarrysMod/bin/libopenvr_api.so
#   install/GarrysMod/bin/linux64/libopenvr_api.so
#   install/GarrysMod/bin/openvr_api.dll
#   install/GarrysMod/bin/openvr_license
#   install/GarrysMod/garrysmod/lua/bin/gmcl_vrmod_linux64.dll
# OpenVR does NOT live next to the module in lua/bin.
rm -f "$MODULE_DIR/libopenvr_api.so"

OPENVR_SO="$ROOT/deps/openvr/lib_linux64/libopenvr_api.so"
cp -f "$OPENVR_SO" "$BIN_DIR/libopenvr_api.so"
cp -f "$OPENVR_SO" "$BIN_LINUX64/libopenvr_api.so"
cp -f "$ROOT/deps/openvr/openvr_api.dll" "$BIN_DIR/openvr_api.dll"
cp -f "$ROOT/deps/openvr/openvr_license" "$BIN_DIR/openvr_license"

echo "Install tree:"
find "$INSTALL_GM" -type f | sort
