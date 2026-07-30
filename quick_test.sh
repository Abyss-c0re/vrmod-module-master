#!/usr/bin/env bash
# Battle-proven flow adapted from gVRMod/quick_test.sh for OpenVR linux_dev.
# Changes vs gVRMod: OpenVR module install (no OpenXR bundle), soldier-runtime
# launch (bypasses Steam ShowInterstitials hang), console screenshots, safe kill.
set -uo pipefail

if ! command -v xdotool >/dev/null 2>&1 || ! command -v wmctrl >/dev/null 2>&1; then
    echo "ERROR: need xdotool + wmctrl"
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

STEAM="${STEAM_DIR:-$HOME/.local/share/Steam}"
GAME_DIR="$STEAM/steamapps/common/GarrysMod"
LIVE_BIN="$GAME_DIR/garrysmod/lua/bin/gmcl_vrmod_linux64.dll"
LIVE_CFG_DIR="$GAME_DIR/garrysmod/cfg"
DEV_DLL="install/GarrysMod/garrysmod/lua/bin/gmcl_vrmod_linux64.dll"
OUT_DIR="${SCRIPT_DIR}/../agent_ops/vrmod_loop"
STAMP=$(date +%Y%m%d_%H%M%S)
RUN_DIR="$OUT_DIR/gvrmod_style_${STAMP}"
mkdir -p "$RUN_DIR"

log() { echo "[$(date +%H:%M:%S)] $*"; }

kill_gmod_safe() {
    # Never pkill -f patterns that match this shell's command line.
    local p
    for p in $(ps -eo pid,comm | awk '$2=="gmod"{print $1}'); do
        kill -TERM "$p" 2>/dev/null || true
    done
    sleep 0.6
    for p in $(ps -eo pid,comm | awk '$2=="gmod"{print $1}'); do
        kill -9 "$p" 2>/dev/null || true
    done
    # reaper children
    for p in $(ps -eo pid,args | awk '/SteamLaunch AppId=4000/ && !/awk/{print $1}'); do
        kill -9 "$p" 2>/dev/null || true
    done
}

find_gmod_win() {
    local WIN="" PID=""
    PID=$(ps -eo pid,comm | awk '$2=="gmod"{print $1; exit}')
    if [ -n "$PID" ]; then
        WIN=$(xdotool search --onlyvisible --pid "$PID" 2>/dev/null | head -1 || true)
    fi
    if [ -z "$WIN" ]; then
        WIN=$(xdotool search --onlyvisible --name "Garry's Mod - OpenGL" 2>/dev/null | head -1 || true)
    fi
    if [ -z "$WIN" ]; then
        WIN=$(xdotool search --onlyvisible --name "Garry's Mod" 2>/dev/null | head -1 || true)
    fi
    if [ -z "$WIN" ]; then
        WIN=$(xdotool search --onlyvisible --class "gmod" 2>/dev/null | head -1 || true)
    fi
    if [ -z "$WIN" ]; then
        WIN=$(wmctrl -l 2>/dev/null | grep -iE "Garry's Mod - OpenGL|Garry's Mod" | grep -vi 'Installed Files\|Properties' | head -1 | awk '{print $1}' || true)
    fi
    echo "$WIN"
}

echo "=== OpenVR linux_dev Quick Test (gVRMod-style) ==="
echo "Building..."
if ! ./build.sh; then
  echo "ERROR: build failed — abort (will not install stale dll)"
  exit 1
fi

if [[ ! -f "$DEV_DLL" ]]; then
  echo "ERROR: $DEV_DLL not found after build"
  exit 1
fi

if [[ -f "$LIVE_BIN" ]]; then
  TS=$(date +%Y%m%d-%H%M%S)
  cp -f "$LIVE_BIN" "${LIVE_BIN}.bak-${TS}"
  echo "Backed up previous dll to ${LIVE_BIN}.bak-${TS}"
fi

echo "Installing OpenVR module (canonical layout: OpenVR in bin/, module in lua/bin)..."
cp -f "$DEV_DLL" "$LIVE_BIN"
# OpenVR lives under game bin/ (not next to the module)
mkdir -p "$GAME_DIR/bin/linux64"
cp -f deps/openvr/lib_linux64/libopenvr_api.so "$GAME_DIR/bin/libopenvr_api.so"
cp -f deps/openvr/lib_linux64/libopenvr_api.so "$GAME_DIR/bin/linux64/libopenvr_api.so"
if [ -f deps/openvr/openvr_api.dll ]; then
  cp -f deps/openvr/openvr_api.dll "$GAME_DIR/bin/openvr_api.dll"
fi
if [ -f deps/openvr/openvr_license ]; then
  cp -f deps/openvr/openvr_license "$GAME_DIR/bin/openvr_license"
fi
# Remove mis-bundled openvr from lua/bin if present
rm -f "$GAME_DIR/garrysmod/lua/bin/libopenvr_api.so"
echo "Installed: $(md5sum "$LIVE_BIN")"

echo "Cleaning logs..."
: > "$GAME_DIR/vrmod_debug.log" 2>/dev/null || true
: > "$GAME_DIR/garrysmod/console.log" 2>/dev/null || true

mkdir -p "$LIVE_CFG_DIR"
cat > "$LIVE_CFG_DIR/vrmod_quicktest.cfg" << 'CFG'
vrmod_desktopview 2
echo "=== VRMOD_QUICKTEST: desktopview set, waiting for focused vrmod_start ==="
CFG

echo "Killing any running GMod..."
kill_gmod_safe
sleep 0.8

# SteamVR (comm may be truncated — do not use pgrep -x)
if ! pgrep vrcompositor >/dev/null 2>&1; then
  echo "Starting SteamVR..."
  steam -applaunch 250820 >/dev/null 2>&1 &
  for i in $(seq 1 40); do
    pgrep vrcompositor >/dev/null 2>&1 && break
    sleep 1
  done
fi
pgrep vrcompositor >/dev/null 2>&1 && echo "SteamVR OK" || echo "WARN: SteamVR not detected"

echo "Launching Garry's Mod via soldier runtime (same as Steam, skips interstitial hang)..."
export SteamAppId=4000 SteamGameId=4000 SteamOverlayGameId=4000
nohup "$STEAM/ubuntu12_32/steam-launch-wrapper" -- \
  "$STEAM/ubuntu12_32/reaper" SteamLaunch AppId=4000 -- \
  "$STEAM/steamapps/common/SteamLinuxRuntime_soldier"/_v2-entry-point --verb=waitforexitandrun -- \
  "$STEAM/steamapps/common/SteamLinuxRuntime"/scout-on-soldier-entry-point-v2 -- \
  "$GAME_DIR/hl2.sh" -steam -game garrysmod -console -condebug \
  +map gm_construct +exec vrmod_quicktest.cfg +sv_cheats 1 \
  > "$RUN_DIR/launch.log" 2>&1 &

echo "Capturing GMod game PID(s)..."
GAME_PIDS=""
for _ in $(seq 1 60); do
    P=$(ps -eo pid,comm | awk '$2=="gmod"{print $1; exit}')
    if [ -n "$P" ]; then
        GAME_PIDS="$P"
        break
    fi
    sleep 0.5
done
if [ -n "$GAME_PIDS" ]; then
    echo "Captured GMod PID(s): $GAME_PIDS"
else
    echo "WARNING: Could not capture GMod PID yet"
fi

echo ">>> GMod window MUST be focused for ticks / vrmod_start <<<"

echo "Waiting for map to load (console.log signals)..."
for i in $(seq 1 120); do
  loaded=0
  if [ -f "$GAME_DIR/garrysmod/console.log" ]; then
    # Need real map progress — window alone is not enough (injects too early)
    if grep -qiE "Redownloading all lightmaps|spawn protected|gm_construct" "$GAME_DIR/garrysmod/console.log" 2>/dev/null; then
      # Prefer lightmaps/spawn over mere map name in cfg
      if grep -qiE "Redownloading all lightmaps|spawn protected|VRMod Subsystem" "$GAME_DIR/garrysmod/console.log" 2>/dev/null; then
        loaded=1
      elif [ "$i" -gt 25 ]; then
        loaded=1
      fi
    fi
  fi
  if [ "$loaded" -eq 1 ]; then
    echo "Map appears loaded at ${i}s"
    break
  fi
  if [ $(( i % 10 )) -eq 0 ]; then
    echo "  ... still waiting for map ($i/120) size=$(wc -c < "$GAME_DIR/garrysmod/console.log" 2>/dev/null || echo 0) ..."
  fi
  sleep 1
done

# gVRMod proven settle + extra for GL/Lua hot (ShareTexture timing confession)
sleep 10

echo "Trying to auto-focus the GMod window (xdotool + wmctrl fallback)..."
FOCUSED=0
WIN=""
for i in $(seq 1 90); do
    WIN=$(find_gmod_win)
    if [ -n "$WIN" ]; then
        xdotool windowactivate --sync "$WIN" 2>/dev/null || true
        xdotool windowraise "$WIN" 2>/dev/null || true
        xdotool windowfocus --sync "$WIN" 2>/dev/null || true
        if [[ "$WIN" == 0x* ]]; then
            wmctrl -i -a "$WIN" 2>/dev/null || true
        fi
        echo "Auto-focused GMod window (ID: $WIN)"
        FOCUSED=1
        break
    fi
    sleep 0.5
done

if [ "$FOCUSED" -eq 0 ]; then
    echo "Could not auto-focus — waiting 12s for manual focus..."
    sleep 12
    WIN=$(find_gmod_win)
fi

# Fresh window id before inject (gVRMod proven)
WIN=""
for _ in $(seq 1 8); do
    WIN=$(find_gmod_win)
    [ -n "$WIN" ] && break
    sleep 0.3
done

if [ -n "${WIN:-}" ]; then
    echo "Window focused. Opening console with \`, sending vrmod_start..."
    # Extra settle: ShareTexture needs GL hot (confession timing)
    sleep 5

    xdotool windowactivate --sync "$WIN" 2>/dev/null || true
    xdotool windowfocus --sync "$WIN" 2>/dev/null || true
    xdotool key --window "$WIN" grave
    sleep 0.6

    xdotool windowactivate --sync "$WIN" 2>/dev/null || true
    xdotool windowfocus --sync "$WIN" 2>/dev/null || true
    xdotool type --window "$WIN" "vrmod_start"
    sleep 0.3
    xdotool windowactivate --sync "$WIN" 2>/dev/null || true
    xdotool windowfocus --sync "$WIN" 2>/dev/null || true
    xdotool key --window "$WIN" Return
    sleep 0.5

    xdotool windowactivate --sync "$WIN" 2>/dev/null || true
    xdotool windowfocus --sync "$WIN" 2>/dev/null || true
    xdotool key --window "$WIN" Escape
    sleep 0.3

    echo "vrmod_start sent. Running 18s for Submit pipeline..."
    sleep 18

    # Screenshot console
    xdotool windowactivate --sync "$WIN" 2>/dev/null || true
    xdotool key --window "$WIN" grave 2>/dev/null || true
    sleep 0.5
    import -window "$WIN" "$RUN_DIR/console.png" 2>/dev/null || true
    import -window root "$RUN_DIR/desktop.png" 2>/dev/null || true
    echo "Screenshots -> $RUN_DIR"

    # Collect logs before kill
    cp -f "$GAME_DIR/vrmod_debug.log" "$RUN_DIR/vrmod_debug.log" 2>/dev/null || true
    cp -f "$GAME_DIR/garrysmod/console.log" "$RUN_DIR/console.log" 2>/dev/null || true

    echo "Terminating GMod..."
    xdotool windowactivate --sync "$WIN" 2>/dev/null || true
    xdotool key --window "$WIN" grave 2>/dev/null || true
    sleep 0.2
    xdotool type --window "$WIN" "quit" 2>/dev/null || true
    xdotool key --window "$WIN" Return 2>/dev/null || true
    sleep 1.2

    if [ -n "${GAME_PIDS:-}" ]; then
        for pid in $GAME_PIDS; do
            kill -TERM "$pid" 2>/dev/null || true
        done
        sleep 0.8
        for pid in $GAME_PIDS; do
            kill -9 "$pid" 2>/dev/null || true
        done
    fi
    kill_gmod_safe
    echo "GMod terminated."
else
    echo "No window handle — skip inject."
    kill_gmod_safe
fi

# Verdict
echo ""
echo "=== VERDICT ==="
ok_n=$(grep -c "Submit ok" "$GAME_DIR/vrmod_debug.log" 2>/dev/null || true); ok_n=$(echo "$ok_n" | tr -d "
"); [ -n "$ok_n" ] || ok_n=0
fail_lines=$(grep -cE "Submit L=|OpenVR Submit failed|TextureUsesUnsupportedFormat|errLeft=105" "$GAME_DIR/vrmod_debug.log" 2>/dev/null || echo 0)
console_fail=$(grep -cE "OpenVR Submit failed|ShareTextureBegin failed|105" "$GAME_DIR/garrysmod/console.log" 2>/dev/null || echo 0)
echo "submit_ok_lines=$ok_n submit_failish=$fail_lines console_failish=$console_fail"
echo "dll=$(md5sum "$LIVE_BIN" | awk '{print $1}')"
echo "run_dir=$RUN_DIR"
echo "--- module key ---"
grep -E "Submit|Share|RGBA|FBO|Captured|armed|mprotect|105" "$GAME_DIR/vrmod_debug.log" 2>/dev/null | tail -40 || true
echo "--- console vrmod ---"
grep -E "vrmod|VRMOD|ShareTexture|Submit|Error" "$GAME_DIR/garrysmod/console.log" 2>/dev/null | tail -30 || true

{
  echo "submit_ok=$ok_n"
  echo "failish=$fail_lines"
  echo "console_failish=$console_fail"
} > "$RUN_DIR/verdict.txt"

ok_n=$(echo "$ok_n" | tr -dc "0-9"); [ -n "$ok_n" ] || ok_n=0
fail_lines=$(echo "$fail_lines" | tr -dc "0-9"); [ -n "$fail_lines" ] || fail_lines=0
console_fail=$(echo "$console_fail" | tr -dc "0-9"); [ -n "$console_fail" ] || console_fail=0
if [ "$ok_n" -ge 3 ] && [ "$fail_lines" -eq 0 ]; then
  echo PASS | tee "$RUN_DIR/result.txt"
  exit 0
fi
echo FAIL | tee "$RUN_DIR/result.txt"
exit 1
