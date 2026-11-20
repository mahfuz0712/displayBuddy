#!/usr/bin/env bash
# Builds DisplayBuddy and collects everything needed to run it on any Windows PC into ./dist
# Run this inside the MSYS2 UCRT64 terminal:   ./deploy-windows.sh
set -euo pipefail
cd "$(dirname "$0")"

BUILD=build-windows
DIST=dist

cmake -S . -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD" --parallel

rm -rf "$DIST"
mkdir -p "$DIST"
cp "$BUILD/displaybuddy.exe" "$DIST/"

# Qt DLLs + plugins (platforms/qwindows.dll etc.)
windeployqt6 --release --no-translations "$DIST/displaybuddy.exe"

# windeployqt on MSYS2 misses some non-Qt DLLs (libstdc++, libpng, harfbuzz, icu, zstd ...).
# Copy every /ucrt64 DLL that the exe, the Qt DLLs or the plugins depend on, until nothing new appears.
changed=1
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