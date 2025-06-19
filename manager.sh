#!/bin/bash

# === CONFIGURATION ===
BUILD_SCRIPT="./build.sh"
INSTALL_DIR="./install"
BACKUP_DIR="./backup"
GAME_DEST="$HOME/.steam/steam/steamapps/common/GarrysMod"

timestamp() {
    date +"%Y%m%d_%H%M%S"
}

run_build() {
    echo "Running build..."
    BUILD_OUTPUT=$($BUILD_SCRIPT 2>&1)

    if [ -n "$BUILD_OUTPUT" ]; then
        echo "❌ Build failed or produced output:"
        echo "$BUILD_OUTPUT"
        exit 1
    fi

    echo "✅ Build completed with no output."
}

create_modules_zip() {
    echo "Creating modules.zip from $INSTALL_DIR/GarrysMod..."
    zip -r -q "./modules.zip" "$INSTALL_DIR/GarrysMod"
    echo "✅ modules.zip created."
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
        rsync -a "$source_path/" "$dest_path/"
        echo "✅ $folder moved successfully."
    else
        echo "⚠️ Folder $folder not found in $source_path"
    fi
}

deploy_to_game() {
    echo "Starting deployment to Garry's Mod..."
    move_and_overwrite "bin"
    move_and_overwrite "garrysmod"
}

restore_backup() {
    echo "Available backups:"
    select BACKUP_FILE in "$BACKUP_DIR"/*.tar.gz; do
        if [ -n "$BACKUP_FILE" ]; then
            echo "Selected: $BACKUP_FILE"
            TEMP_DIR=$(mktemp -d)
            echo "Extracting to $TEMP_DIR..."
            tar -xzf "$BACKUP_FILE" -C "$TEMP_DIR"
            echo "Deploying restored backup..."
            INSTALL_DIR="$TEMP_DIR"
            deploy_to_game
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

# === MAIN ENTRY POINT ===

case "$1" in
    restore)
        restore_backup
        ;;
    uninstall)
        uninstall
        ;;
    *)
        run_build
        create_modules_zip
        backup_install
        deploy_to_game
        ;;
esac
