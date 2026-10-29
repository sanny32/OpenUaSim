#!/usr/bin/env bash
set -euo pipefail

source_root="$(cd "$(dirname "$0")/../../.." && pwd)"
fixture="$(mktemp -d)"
trap 'rm -rf "$fixture"' EXIT
mkdir -p "$fixture/bin" "$fixture/src" "$fixture/elf tree"
cp "$source_root/src/CMakeLists.txt" "$fixture/src/"
cat > "$fixture/bin/git" <<'EOF'
#!/usr/bin/env bash
case "$*" in
    *rev-parse*)
        [[ ${FAIL_REV_PARSE:-0} == 0 ]] || exit 1
        echo abc1234 ;;
    *log*) echo 2026-10-29 ;;
    *) exit 1 ;;
esac
EOF
cat > "$fixture/bin/objdump" <<'EOF'
#!/usr/bin/env bash
cat "$2"
EOF
chmod +x "$fixture/bin/"*
export PATH="$fixture/bin:$PATH"
cd "$fixture"

expect_failure() {
    if "$@" > "$fixture/error.log" 2>&1; then
        echo "Unexpected success: $*" >&2
        exit 1
    fi
}

version="$(sed -nE 's/^[[:space:]]*VERSION ([0-9.]+)$/\1/p' src/CMakeLists.txt)"
GITHUB_REF="refs/tags/v$version" bash "$source_root/tools/package_version.sh" > version.txt
grep -Fx "appVersion=$version" version.txt
grep -Fx "prerelease=" version.txt
grep -Fx "debVersion=$version-1" version.txt
grep -Fx "fileVersion=$version.0" version.txt

GITHUB_REF="refs/tags/v$version-rc1" bash "$source_root/tools/package_version.sh" > version.txt
grep -Fx "appVersion=$version-rc1" version.txt
grep -Fx "debVersion=$version~rc1-1" version.txt
grep -Fx "rpmVersion=$version~rc1" version.txt

GITHUB_REF=refs/heads/main bash "$source_root/tools/package_version.sh" > version.txt
grep -Fx "appVersion=$version-gabc1234" version.txt
grep -Fx "prerelease=gabc1234" version.txt
grep -Fx "debVersion=$version~gabc1234-1" version.txt
grep -Fx "rpmVersion=$version~gabc1234" version.txt
grep -Fx "fileVersion=$version.0" version.txt
expect_failure env FAIL_REV_PARSE=1 GITHUB_REF=refs/heads/main bash "$source_root/tools/package_version.sh"

for ref in refs/heads/feature/test ''; do
    GITHUB_REF="$ref" bash "$source_root/tools/package_version.sh" > version.txt
    grep -Fx "appVersion=$version" version.txt
    grep -Fx "prerelease=" version.txt
    grep -Fx "debVersion=$version-1" version.txt
    grep -Fx "rpmVersion=$version" version.txt
    grep -Fx "fileVersion=$version.0" version.txt
done

expect_failure env GITHUB_REF=refs/tags/v99.0.0 bash "$source_root/tools/package_version.sh"
expect_failure env GITHUB_REF="refs/tags/v$version-rc/1" bash "$source_root/tools/package_version.sh"
expect_failure env GITHUB_REF=refs/tags/vgarbage bash "$source_root/tools/package_version.sh"
printf 'project(ouasim VERSION invalid)\n' > src/CMakeLists.txt
expect_failure bash "$source_root/tools/package_version.sh"

expect_failure bash "$source_root/tools/check_glibc_floor.sh" missing
expect_failure bash "$source_root/tools/check_glibc_floor.sh" 'elf tree'
printf '0000 GLIBC_2.2.5 symbol\n0001 GLIBC_2.28 symbol\n' > 'elf tree/ouasim'
bash "$source_root/tools/check_glibc_floor.sh" 'elf tree'
printf 'No versioned symbols\n' > 'elf tree/libdata.so'
bash "$source_root/tools/check_glibc_floor.sh" 'elf tree'
printf '0000 GLIBC_2.9 symbol\n' > 'elf tree/libold.so'
bash "$source_root/tools/check_glibc_floor.sh" 'elf tree'
printf '0000 GLIBC_2.29 symbol\n' > 'elf tree/libnew.so'
expect_failure bash "$source_root/tools/check_glibc_floor.sh" 'elf tree'
grep -F 'libnew.so needs GLIBC_2.29' "$fixture/error.log"
expect_failure bash "$source_root/tools/check_glibc_floor.sh" 'elf tree' invalid

expect_failure bash "$source_root/.github/linux/bundle.sh"
expect_failure bash "$source_root/.github/linux/bundle.sh" stage missing qt openssl
if command -v cmake > /dev/null; then
    cmake "-DOUASIM_SOURCE_ROOT=$source_root" "-DFIXTURE_ROOT=$fixture" \
        -P "$source_root/src/tests/buildscripts/test_deploy.cmake"
    if command -v ninja > /dev/null; then
        cmake "-DOUASIM_SOURCE_ROOT=$source_root" "-DFIXTURE_ROOT=$fixture" \
            "-DOUASIM_MAKE_PROGRAM=$(command -v ninja)" \
            -P "$source_root/src/tests/buildscripts/test_install.cmake"
    else
        echo 'Skipping installation layout tests: Ninja unavailable'
    fi
else
    echo 'Skipping deployment failure tests: CMake unavailable'
fi
echo 'Packaging script tests passed'
bash "$source_root/src/tests/buildscripts/test_linux_bundle.sh"
