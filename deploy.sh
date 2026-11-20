#!/usr/bin/env bash
# DisplayBuddy build + packaging script (replaces deployWindows.sh / deployLinux.sh).
#
#   ./deploy.sh --windows   Run inside the MSYS2 UCRT64 terminal.
#                           Builds the app and collects everything needed to run it on any
#                           Windows PC into ./dist  (then build setup/setup.iss with Inno Setup).
#   ./deploy.sh --linux     Run on Ubuntu/Debian/Fedora etc.
#                           Builds the app and stages an installable tree into ./dist-linux
#                           (bin/, share/applications, share/icons).
#   ./deploy.sh --help
set -euo pipefail
cd "$(dirname "$0")"

usage() {
    sed -n '2,10p' "$0" | sed 's/^# \{0,1\}//'
}

deploy_windows() {
    local BUILD=build-windows
    local DIST=dist

    if ! command -v windeployqt6 >/dev/null 2>&1; then
        echo "error: windeployqt6 not found. Run this in the MSYS2 UCRT64 terminal (not MSYS or MINGW64)." >&2
        exit 1
    fi

    cmake -S . -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build "$BUILD" --parallel

    rm -rf "$DIST"
    mkdir -p "$DIST"
    cp "$BUILD/displaybuddy.exe" "$DIST/"

    # Qt DLLs + plugins (platforms/qwindows.dll etc.)
    windeployqt6 --release --no-translations "$DIST/displaybuddy.exe"

    # windeployqt on MSYS2 misses some non-Qt DLLs (libstdc++, libpng, harfbuzz, icu, zstd ...).
    # Copy every /ucrt64 DLL that the exe, the Qt DLLs or the plugins depend on, until nothing new appears.
    local changed=1 file dep
    while [ "$changed" -eq 1 ]; do
        changed=0
        while IFS= read -r file; do
            while IFS= read -r dep; do
                [ -z "$dep" ] && continue
                if [ ! -f "$DIST/$(basename "$dep")" ]; then
                    cp "$dep" "$DIST/"
                    changed=1
                fi
            done < <(ldd "$file" 2>/dev/null | awk '$2 == "=>" && $3 ~ /^\/ucrt64\// {print $3}')
        done < <(find "$DIST" -type f \( -name '*.dll' -o -name '*.exe' \))
    done

    echo
    echo "Done. Portable app folder: $(pwd)/$DIST  ($(du -sh "$DIST" | cut -f1))"
    echo "Test it:  ./$DIST/displaybuddy.exe"
    echo "Then open setup/setup.iss in Inno Setup and press Ctrl+F9 to build the setup.exe"
}

deploy_linux() {
    local BUILD=build-linux
    local DIST=dist-linux

    if [ "$(uname -s)" != "Linux" ]; then
        echo "error: --linux must be run on Linux (use --windows in MSYS2 for the Windows build)." >&2
        exit 1
    fi

    cmake -S . -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build "$BUILD" --parallel

    rm -rf "$DIST"
    cmake --install "$BUILD" --prefix "$(pwd)/$DIST"

    echo
    echo "Done. Staged install tree: $(pwd)/$DIST  ($(du -sh "$DIST" | cut -f1))"
    echo "Test it:     ./$DIST/bin/displaybuddy"
    echo "Install it:  sudo cmake --install $BUILD --prefix /usr/local"
}

case "${1:-}" in
    --windows|-w) deploy_windows ;;
    --linux|-l)   deploy_linux ;;
    -h|--help)    usage ;;
    *)
        usage >&2
        exit 1
        ;;
esac