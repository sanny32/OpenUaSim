#!/usr/bin/env bash
set -euo pipefail

version="$(sed -nE '/^project\(ouasim([[:space:]]|$)/,/\)/ s/.*VERSION[[:space:]]+([0-9]+\.[0-9]+\.[0-9]+).*/\1/p' src/CMakeLists.txt)"
[[ $version =~ ^([0-9]+)\.([0-9]+)\.([0-9]+)$ ]] || { echo 'Invalid CMake project version' >&2; exit 1; }
file_version="$version.0"
prerelease="dev.g$(git -c safe.directory="$(pwd)" rev-parse --short HEAD)"
if [[ ${GITHUB_REF:-} == refs/tags/* ]]; then
    tag="${GITHUB_REF#refs/tags/}"
    [[ $tag =~ ^v([0-9]+\.[0-9]+\.[0-9]+)(-([0-9A-Za-z]+([.-][0-9A-Za-z]+)*))?$ ]] || {
        echo "Invalid release tag: $tag" >&2; exit 1;
    }
    [[ ${BASH_REMATCH[1]} == "$version" ]] || { echo 'Tag does not match CMake project version' >&2; exit 1; }
    prerelease="${BASH_REMATCH[3]}"
fi
app_version="$version${prerelease:+-$prerelease}"
package_version="$version${prerelease:+~$prerelease}"
printf 'appVersion=%s\nprerelease=%s\ndebVersion=%s-1\nrpmVersion=%s\nrpmRelease=1\nfileVersion=%s\nreleaseDate=%s\n' \
    "$app_version" "$prerelease" "$package_version" "${package_version//-/.}" "$file_version" \
    "$(git -c safe.directory="$(pwd)" log -1 --format=%cs)"
