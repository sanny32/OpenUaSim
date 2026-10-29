#!/usr/bin/env bash

set -euo pipefail

if [ "$#" -ne 4 ]; then
    echo "Usage: $0 <stage-dir> <executable> <qt-prefix> <openssl-prefix>" >&2
    exit 1
fi

STAGE_DIR="$1"
EXECUTABLE="$2"
QT_PREFIX="$3"
OPENSSL_PREFIX="$4"

APP_NAME=ouasim
APP_ROOT="$STAGE_DIR/usr/lib/$APP_NAME"
APP_BIN_DIR="$APP_ROOT/bin"
APP_LIB_DIR="$APP_ROOT/lib"
APP_PLUGIN_DIR="$APP_ROOT/plugins"
APP_INSTALLED_LIB_DIR="/usr/lib/$APP_NAME/lib"
APP_BIN_RPATH="$APP_INSTALLED_LIB_DIR:\$ORIGIN/../lib"
APP_LIB_RPATH="$APP_INSTALLED_LIB_DIR:\$ORIGIN"
APP_PLUGIN_RPATH="$APP_INSTALLED_LIB_DIR:\$ORIGIN/../../lib"

QT_PLUGIN_GROUPS=(
    platforms
    platformthemes
    platforminputcontexts
    xcbglintegrations
    egldeviceintegrations
    imageformats
    iconengines
    networkinformation
    tls
    styles
)

log() {
    printf '\033[32m-- %s\033[0m\n' "$*"
}

die() {
    printf '\033[31m** %s\033[0m\n' "$*" >&2
    exit 1
}

[ -x "$EXECUTABLE" ]     || die "Executable not found: $EXECUTABLE"
[ -d "$QT_PREFIX/lib" ]  || die "Qt prefix has no lib directory: $QT_PREFIX"

if [ -n "$OPENSSL_PREFIX" ]; then
    [ -d "$OPENSSL_PREFIX/lib" ] \
        || die "OpenSSL prefix has no lib directory: $OPENSSL_PREFIX"
fi

command -v patchelf >/dev/null || die "patchelf is required"

rm -rf "$APP_ROOT"
mkdir -p "$APP_BIN_DIR" "$APP_LIB_DIR" "$APP_PLUGIN_DIR"

log "Copying the executable"
install -m 0755 "$EXECUTABLE" "$APP_BIN_DIR/$APP_NAME"

cat > "$APP_BIN_DIR/qt.conf" <<'EOF'
[Paths]
Prefix = ..
Libraries = lib
Plugins = plugins
EOF
chmod 0644 "$APP_BIN_DIR/qt.conf"

log "Copying Qt plugins"
for group in "${QT_PLUGIN_GROUPS[@]}"; do
    if [ -d "$QT_PREFIX/plugins/$group" ]; then
        cp -a "$QT_PREFIX/plugins/$group" "$APP_PLUGIN_DIR/"
    fi
done

find "$APP_PLUGIN_DIR" -type f -name '*.debug' -delete

if [ -n "$OPENSSL_PREFIX" ]; then
    log "Copying OpenSSL"
    for lib in libcrypto libssl; do
        path="$(find "$OPENSSL_PREFIX/lib" -maxdepth 1 -type f -name "$lib.so.*" -print -quit)"
        [ -n "$path" ] || die "No $lib in $OPENSSL_PREFIX/lib"
        soname="$(patchelf --print-soname "$path")"
        install -m 0644 "$path" "$APP_LIB_DIR/$soname"
        log "  bundled $soname"
    done
fi

log "Resolving private library dependencies"

is_private() {
    case "$1" in
        "$QT_PREFIX"/*) return 0 ;;
    esac
    if [ -n "$OPENSSL_PREFIX" ]; then
        case "$1" in
            "$OPENSSL_PREFIX"/*) return 0 ;;
        esac
    fi
    return 1
}

elf_files() {
    find "$APP_BIN_DIR" "$APP_LIB_DIR" "$APP_PLUGIN_DIR" -type f \
        \( -name '*.so' -o -name '*.so.*' -o -name "$APP_NAME" \) -print
}


export LD_LIBRARY_PATH="$QT_PREFIX/lib:$OPENSSL_PREFIX/lib:${LD_LIBRARY_PATH:-}"

copied_any=1
while [ "$copied_any" -eq 1 ]; do
    copied_any=0
    while read -r elf; do
        while read -r needed path; do
            [ -n "$path" ] || continue
            is_private "$path" || continue
            [ -e "$APP_LIB_DIR/$needed" ] && continue
            cp -L "$path" "$APP_LIB_DIR/$needed"
            chmod 0644 "$APP_LIB_DIR/$needed"
            log "  bundled $needed"
            copied_any=1
        done < <(ldd "$elf" | sed -nE 's|^[[:space:]]*([^[:space:]]+) => (/[^[:space:]]+).*|\1 \2|p')
    done < <(elf_files)
done

# Strip before patchelf so section rewrites preserve Qt plugin metadata.
log "Stripping binaries"
strip --strip-unneeded "$APP_BIN_DIR/$APP_NAME"
find "$APP_LIB_DIR" "$APP_PLUGIN_DIR" -type f -name '*.so*' \
    -exec strip --strip-unneeded {} +

# DT_RPATH also resolves transitive dependencies from the private library tree.
log "Rewriting RPATHs"
patchelf --force-rpath --set-rpath "$APP_BIN_RPATH" "$APP_BIN_DIR/$APP_NAME"
find "$APP_LIB_DIR" -type f -name '*.so*' -exec patchelf --force-rpath --set-rpath "$APP_LIB_RPATH" {} \;
find "$APP_PLUGIN_DIR" -type f -name '*.so' -exec patchelf --force-rpath --set-rpath "$APP_PLUGIN_RPATH" {} \;

find "$APP_LIB_DIR" "$APP_PLUGIN_DIR" -type f -exec chmod 0644 {} +
chmod 0755 "$APP_BIN_DIR/$APP_NAME"

mkdir -p "$STAGE_DIR/usr/bin"
ln -sfn "../lib/$APP_NAME/bin/$APP_NAME" "$STAGE_DIR/usr/bin/$APP_NAME"

log "Checking that every library resolves through the bundle"
unset LD_LIBRARY_PATH
unresolved=0
while read -r elf; do
    if ldd "$elf" | grep -q 'not found'; then
        printf '\033[31m** unresolved libraries in %s\033[0m\n' "$elf" >&2
        ldd "$elf" | grep 'not found' >&2
        unresolved=1
    fi
done < <(elf_files)
[ "$unresolved" -eq 0 ] || die "The bundle is incomplete"

log "Bundle staged in $APP_ROOT"
