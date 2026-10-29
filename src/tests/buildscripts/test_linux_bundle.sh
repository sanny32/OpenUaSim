#!/usr/bin/env bash
set -euo pipefail

if [[ $(uname -s) != Linux ]]; then
    echo 'Skipping ELF bundle test outside Linux'
    exit 0
fi
for tool in cc patchelf strip ldd; do
    if ! command -v "$tool" > /dev/null; then
        echo "Skipping ELF bundle test: $tool unavailable"
        exit 0
    fi
done

source_root="$(cd "$(dirname "$0")/../../.." && pwd)"
fixture="$(mktemp -d)"
trap 'rm -rf "$fixture"' EXIT
mkdir -p "$fixture/qt/lib" "$fixture/qt/plugins/platforms" "$fixture/openssl/lib"
cd "$fixture"
printf 'int qt_value(void) { return 42; }\n' > qt.c
printf 'int crypto_value(void) { return 1; }\n' > crypto.c
printf 'int ssl_value(void) { return 1; }\n' > ssl.c
printf 'extern int qt_value(void); int plugin(void) { return qt_value(); }\n' > plugin.c
printf 'extern int qt_value(void); extern int crypto_value(void); int main(void) { return qt_value() + crypto_value() == 43 ? 0 : 1; }\n' > app.c
cc -shared -fPIC -Wl,-soname,libQt6Core.so.6 -o qt/lib/libQt6Core.so.6 qt.c
cc -shared -fPIC -Wl,-soname,libcrypto.so.3 -o openssl/lib/libcrypto.so.3 crypto.c
cc -shared -fPIC -Wl,-soname,libssl.so.3 -o openssl/lib/libssl.so.3 ssl.c
cc -shared -fPIC -o qt/plugins/platforms/libqtest.so plugin.c -Lqt/lib -l:libQt6Core.so.6
cc -o app app.c -Lqt/lib -l:libQt6Core.so.6 -Lopenssl/lib -l:libcrypto.so.3

bash "$source_root/.github/linux/bundle.sh" "$fixture/stage" "$fixture/app" "$fixture/qt" "$fixture/openssl"
test -L stage/usr/bin/ouasim
test -f stage/usr/lib/ouasim/lib/libQt6Core.so.6
test -f stage/usr/lib/ouasim/lib/libcrypto.so.3
test -f stage/usr/lib/ouasim/lib/libssl.so.3
test -f stage/usr/lib/ouasim/plugins/platforms/libqtest.so
test -f stage/usr/lib/ouasim/bin/qt.conf
grep -Fx 'Prefix = ..' stage/usr/lib/ouasim/bin/qt.conf
grep -Fx 'Libraries = lib' stage/usr/lib/ouasim/bin/qt.conf
grep -Fx 'Plugins = plugins' stage/usr/lib/ouasim/bin/qt.conf
test ! -e stage/usr/lib/ouasim/lib/libc.so.6

for group in platformthemes platforminputcontexts xcbglintegrations egldeviceintegrations imageformats iconengines networkinformation tls styles; do
    mkdir -p "qt/plugins/$group"
    cp qt/plugins/platforms/libqtest.so "qt/plugins/$group/libqtest.so"
done
printf 'unused debug symbols' > qt/plugins/platforms/libqtest.so.debug
bash "$source_root/.github/linux/bundle.sh" "$fixture/stage" "$fixture/app" "$fixture/qt" "$fixture/openssl"
for group in platforms platformthemes platforminputcontexts xcbglintegrations egldeviceintegrations imageformats iconengines networkinformation tls styles; do
    test -f "stage/usr/lib/ouasim/plugins/$group/libqtest.so"
done
test ! -e stage/usr/lib/ouasim/plugins/platforms/libqtest.so.debug
mv qt qt-hidden
mv openssl openssl-hidden
stage/usr/bin/ouasim
mv stage relocated
relocated/usr/bin/ouasim

expect_failure() {
    if bash "$source_root/.github/linux/bundle.sh" "$fixture/failure" "$fixture/app" "$fixture/qt-hidden" "$fixture/openssl-hidden" > error.log 2>&1; then
        echo 'Unexpected success with incomplete OpenSSL' >&2
        exit 1
    fi
}
mv openssl-hidden/lib/libssl.so.3 ssl-hidden.so
expect_failure
grep -F 'No libssl' error.log
echo 'ELF bundle tests passed'
