#!/usr/bin/env bash

set -euo pipefail

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <openssl-source-dir> <install-prefix>" >&2
    exit 1
fi

SOURCE_ROOT="$1"
PREFIX="$2"

log() {
    printf '\033[32m-- %s\033[0m\n' "$*"
}

die() {
    printf '\033[31m** %s\033[0m\n' "$*" >&2
    exit 1
}

if [ -e "$PREFIX/lib/libssl.so.3" ] && [ -e "$PREFIX/lib/libcrypto.so.3" ]; then
    log "OpenSSL is already built in $PREFIX"
    exit 0
fi

[ -d "$SOURCE_ROOT" ] || die "OpenSSL source directory not found: $SOURCE_ROOT"
if [ -x "$SOURCE_ROOT/Configure" ]; then
    SOURCE_DIR="$SOURCE_ROOT"
else
    SOURCE_DIR="$(
        find "$SOURCE_ROOT" -maxdepth 3 -name Configure -type f -printf '%h\n' -quit
    )"
    [ -n "$SOURCE_DIR" ] || die "No OpenSSL Configure script under $SOURCE_ROOT"
fi

log "Building OpenSSL from $SOURCE_DIR"
(
    cd "$SOURCE_DIR"
    ./Configure \
        --prefix="$PREFIX" \
        --libdir=lib \
        --openssldir=/etc/ssl \
        shared \
        no-tests
    make -j"$(nproc)"
    make install_sw
)

[ -e "$PREFIX/lib/libssl.so.3" ] || die "The build produced no libssl.so.3"
[ -e "$PREFIX/lib/libcrypto.so.3" ] || die "The build produced no libcrypto.so.3"

log "OpenSSL installed in $PREFIX"
