#!/bin/bash
# Install / package / version-control manager for VRMod modules.
# Canonical package: /home/voldemar/Desktop/modules.zip
# Layout must match reference modules.zip:
#   install/GarrysMod/bin/{libopenvr_api.so,openvr_api.dll,openvr_license,linux64/libopenvr_api.so}
#   install/GarrysMod/garrysmod/lua/bin/gmcl_vrmod_linux64.dll

set -euo pipefail

# === CONFIGURATION ===
ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

BUILD_SCRIPT="./build.sh"
INSTALL_DIR="./install"
BACKUP_DIR="./backup"
MODULES_ZIP="${MODULES_ZIP:-$HOME/Desktop/modules.zip}"
GAME_DEST="${GAME_DEST:-$HOME/.steam/steam/steamapps/common/GarrysMod}"
# Alternate Steam path used on some installs
if [ ! -d "$GAME_DEST" ] && [ -d "$HOME/.local/share/Steam/steamapps/common/GarrysMod" ]; then
    GAME_DEST="$HOME/.local/share/Steam/steamapps/common/GarrysMod"
fi

timestamp() {
    date +"%Y%m%d_%H%M%S"
}

run_build() {
    echo "Running build..."
    set +e
    BUILD_OUTPUT=$($BUILD_SCRIPT 2>&1)
    BUILD_RC=$?
    set -e

    if [ $BUILD_RC -ne 0 ]; then
        echo "❌ Build failed (exit $BUILD_RC):"
        echo "$BUILD_OUTPUT"
        exit 1
    fi

    echo "✅ Build completed."
    # Quiet summary only (full cmake/make log on failure)
    echo "$BUILD_OUTPUT" | tail -n 20
}

create_modules_zip() {
    echo "Creating modules.zip from $INSTALL_DIR/GarrysMod..."
    if [ ! -d "$INSTALL_DIR/GarrysMod" ]; then
        echo "❌ Missing $INSTALL_DIR/GarrysMod — run build first."
        exit 1
    fi

    # Validate canonical layout before packaging
    local missing=0
    for f in \
        "$INSTALL_DIR/GarrysMod/garrysmod/lua/bin/gmcl_vrmod_linux64.dll" \
        "$INSTALL_DIR/GarrysMod/bin/libopenvr_api.so" \
        "$INSTALL_DIR/GarrysMod/bin/linux64/libopenvr_api.so" \
        "$INSTALL_DIR/GarrysMod/bin/openvr_api.dll" \
        "$INSTALL_DIR/GarrysMod/bin/openvr_license"
    do
        if [ ! -f "$f" ]; then
            echo "❌ Missing required package file: $f"
            missing=1
        fi
    done
    if [ -f "$INSTALL_DIR/GarrysMod/garrysmod/lua/bin/libopenvr_api.so" ]; then
        echo "❌ OpenVR must not live in lua/bin (found libopenvr_api.so there)."
        missing=1
    fi
    if [ "$missing" -ne 0 ]; then
        exit 1
    fi

    local tmp_zip
    tmp_zip="$(mktemp "${TMPDIR:-/tmp}/modules.XXXXXX.zip")"
    # Zip paths must be install/GarrysMod/... (reference modules.zip format)
    zip -r -q "$tmp_zip" "$INSTALL_DIR/GarrysMod"
    mkdir -p "$(dirname "$MODULES_ZIP")"
    mv -f "$tmp_zip" "$MODULES_ZIP"
    echo "✅ modules.zip created: $MODULES_ZIP"
    unzip -l "$MODULES_ZIP"
}

backup_install() {
    mkdir -p "$BACKUP_DIR"
    TS=$(timestamp)
    BACKUP_NAME="install_backup_$TS.tar.gz"
    echo "Backing up current install to $BACKUP_DIR/$BACKUP_NAME"
    tar -czf "$BACKUP_DIR/$BACKUP_NAME" -C "$INSTALL_DIR" .
    echo "✅ Backup complete."
}

move_and_overwrite() {
    local folder="$1"
    local source_path="$INSTALL_DIR/GarrysMod/$folder"
    local dest_path="$GAME_DEST/$folder"

    if [ -d "$source_path" ]; then
        echo "Moving $folder to $dest_path..."
        mkdir -p "$dest_path"
        rsync -a "$source_path/" "$dest_path/"
        echo "✅ $folder moved successfully."
    else
        echo "⚠️ Folder $folder not found in $source_path"
    fi
}

deploy_to_game() {
    echo "Starting deployment to Garry's Mod..."
    if [ ! -d "$GAME_DEST" ]; then
        echo "❌ Game dest not found: $GAME_DEST"
        exit 1
    fi
    move_and_overwrite "bin"
    move_and_overwrite "garrysmod"
    echo "✅ Deployed to $GAME_DEST"
}

restore_backup() {
    if ! compgen -G "$BACKUP_DIR"/*.tar.gz > /dev/null; then
        echo "❌ No backups in $BACKUP_DIR"
        exit 1
    fi

    echo "Available backups:"
    select BACKUP_FILE in "$BACKUP_DIR"/*.tar.gz; do
        if [ -n "${BACKUP_FILE:-}" ]; then
            echo "Selected: $BACKUP_FILE"
            TEMP_DIR=$(mktemp -d)
            echo "Extracting to $TEMP_DIR..."
            tar -xzf "$BACKUP_FILE" -C "$TEMP_DIR"
            echo "Deploying restored backup..."
            INSTALL_DIR="$TEMP_DIR"
            deploy_to_game
            rm -rf "$TEMP_DIR"
            break
        else
            echo "Invalid selection."
        fi
    done
}

uninstall() {
    echo "Starting uninstall (files only)..."

    for folder in bin garrysmod; do
        local source_path="$INSTALL_DIR/GarrysMod/$folder"
        local dest_path="$GAME_DEST/$folder"

        if [ ! -d "$source_path" ]; then
            echo "⚠️ Source folder $source_path not found, skipping uninstall for $folder."
            continue
        fi

        # Find all files inside source_path only
        find "$source_path" -type f | while read -r file; do
            # Compute relative path from source_path root
            rel_path="${file#$source_path/}"
            target="$dest_path/$rel_path"

            if [ -f "$target" ]; then
                echo "Removing file: $target"
                rm -f "$target"
            fi
        done
    done

    echo "✅ Uninstall completed."
}

usage() {
    cat <<EOF
Usage: $0 [command]

  (default)   build → modules.zip → backup → deploy to game
  zip         build + create $MODULES_ZIP only (no game deploy)
  deploy      deploy current install/ to game (no rebuild)
  restore     restore a backup into the game
  uninstall   remove only files that came from install/
  help        this message

Env:
  MODULES_ZIP   default: \$HOME/Desktop/modules.zip
  GAME_DEST     default: Steam GarrysMod path
EOF
}

# === MAIN ENTRY POINT ===

case "${1:-}" in
    restore)
        restore_backup
        ;;
    uninstall)
        uninstall
        ;;
    zip)
        run_build
        create_modules_zip
        ;;
    deploy)
        deploy_to_game
        ;;
    help|-h|--help)
        usage
        ;;
    "")
        run_build
        create_modules_zip
        backup_install
        deploy_to_game
        ;;
    *)
        echo "Unknown command: $1"
        usage
        exit 1
        ;;
esac
