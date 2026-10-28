#!/usr/bin/env bash
set -euo pipefail

source ./build.sh

if [ "${1:-}" = build ]; then
    CMAKE_BIN="$TEST_TOOLS/cmake"
    PROJECT_DIR="$TEST_TOOLS/project with spaces"
    SOURCE_DIR="$PROJECT_DIR/src"
    QT_PREFIX="$TEST_TOOLS/Qt kit"
    QT_VERSION=6.9.3
    RUN_TESTS="$2"
    RUN_INSTALL="$3"
    INSTALL_PREFIX="$TEST_TOOLS/install prefix"
    export OPENSSL_ROOT_DIR="$TEST_TOOLS/OpenSSL root"
    export CC=custom-gcc CXX=custom-g++
    build_project gcc
    exit 0
fi

fail() {
    echo "$*" >&2
    exit 1
}

version_ge 6.10.0 6.9.3 || fail 'Version comparison must be numeric'
version_ge 6.9 6.9.0 || fail 'Missing version components must be zero'
if version_ge 6.8.3 6.9; then fail 'Older Qt must be rejected'; fi
[ "$(required_cmake_version)" = 3.22 ] || fail 'Minimum CMake extraction'
[ "$(required_qt_version)" = 6.9 ] || fail 'Minimum Qt extraction'
[ "$(qt_required_glibc 6.9.3)" = 2.28 ] || fail 'Qt 6.9 glibc requirement'
[ "$(qt_required_glibc 6.10.0)" = 2.34 ] || fail 'Qt 6.10 glibc requirement'

bash build.sh --help >/dev/null
if bash build.sh --unknown >/dev/null 2>&1; then fail 'Unknown option accepted'; fi
if bash build.sh --install= >/dev/null 2>&1; then fail 'Empty install prefix accepted'; fi
if bash tools/build_openssl.sh >/dev/null 2>&1; then fail 'OpenSSL missing arguments accepted'; fi

TEST_TOOLS="$(mktemp -d)"
export TEST_TOOLS
trap 'rm -rf "$TEST_TOOLS"' EXIT
export TEST_LOG="$TEST_TOOLS/commands"

for DISTRO in debian rhel altlinux suse arch; do
    packages="$(linux_qt6_packages)"
    case "$DISTRO" in
        debian) expected='libqt6svg6-dev' ;;
        rhel) expected='qt6-qtsvg-devel' ;;
        altlinux|suse) expected='qt6-svg-devel' ;;
        arch) expected='qt6-svg' ;;
    esac
    printf '%s\n' "$packages" | tr ' ' '\n' | grep -Fxq "$expected" || fail "Missing SVG package for $DISTRO"
    if [[ "$packages" == *charts* ]]; then fail 'Unexpected Qt Charts dependency'; fi
done

TOOLS_DIR="$TEST_TOOLS"
mkdir -p "$TOOLS_DIR/aqt-venv/bin"
cat >"$TOOLS_DIR/aqt-venv/bin/python3" <<'MOCK'
#!/usr/bin/env bash
if [[ " $* " == *' --arch '* ]]; then
    echo linux_gcc_64
else
    echo '6.8.3 6.9.3 6.10.0'
fi
MOCK
chmod +x "$TOOLS_DIR/aqt-venv/bin/python3"
if [ "$(uname -s)" != Darwin ]; then
    glibc_version() { echo 2.28; }
    [ "$(aqt_qt6_versions 6.9)" = 6.9.3 ] || fail 'glibc 2.28 must exclude Qt 6.10'
    glibc_version() { echo 2.34; }
    [ "$(aqt_qt6_versions 6.9)" = $'6.10.0\n6.9.3' ] || fail 'Newest compatible Qt must come first'
fi

cat >"$TEST_TOOLS/cmake" <<'MOCK'
#!/usr/bin/env bash
printf '%s\n' cmake "$@" >>"$TEST_LOG"
if [ "${FAIL_STAGE:-}" = "$1" ]; then exit 7; fi
MOCK
cat >"$TEST_TOOLS/ctest" <<'MOCK'
#!/usr/bin/env bash
printf '%s\n' ctest "$@" >>"$TEST_LOG"
if [ "${FAIL_STAGE:-}" = ctest ]; then exit 8; fi
MOCK
chmod +x "$TEST_TOOLS/cmake" "$TEST_TOOLS/ctest"

bash src/tests/buildscripts/test_build.sh build 0 0
grep -Fxq -- '-DOUASIM_BUILD_TESTS=OFF' "$TEST_LOG" || fail 'Tests should default to OFF'
if grep -Fxq ctest "$TEST_LOG"; then fail 'Tests ran without --tests'; fi
if grep -Fxq -- --install "$TEST_LOG"; then fail 'Unexpected installation'; fi

: >"$TEST_LOG"
bash src/tests/buildscripts/test_build.sh build 1 1
for argument in '-DOUASIM_BUILD_TESTS=ON' '-DCMAKE_BUILD_TYPE=Release' \
    "-DCMAKE_PREFIX_PATH=$TEST_TOOLS/Qt kit" "-DOPENSSL_ROOT_DIR=$TEST_TOOLS/OpenSSL root" \
    '-DCMAKE_C_COMPILER=custom-gcc' '-DCMAKE_CXX_COMPILER=custom-g++' \
    ctest --output-on-failure --no-tests=error ouasim --install "$TEST_TOOLS/install prefix"; do
    grep -Fxq -- "$argument" "$TEST_LOG" || fail "Missing argument: $argument"
done

for stage in -S --build ctest; do
    : >"$TEST_LOG"
    if FAIL_STAGE="$stage" bash src/tests/buildscripts/test_build.sh build 1 1; then
        fail "Failure in $stage was ignored"
    fi
    if grep -Fxq -- --install "$TEST_LOG"; then fail 'Installed after a failed build/test'; fi
done

mkdir -p "$TEST_TOOLS/ssl/lib"
touch "$TEST_TOOLS/ssl/lib/libssl.so.3" "$TEST_TOOLS/ssl/lib/libcrypto.so.3"
bash tools/build_openssl.sh "$TEST_TOOLS/missing source" "$TEST_TOOLS/ssl"
if bash tools/build_openssl.sh "$TEST_TOOLS/missing source" "$TEST_TOOLS/missing prefix"; then
    fail 'Missing OpenSSL sources accepted'
fi

bash src/tests/buildscripts/test_ci_cache.sh
