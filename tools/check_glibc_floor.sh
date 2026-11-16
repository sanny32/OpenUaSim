#!/usr/bin/env bash
set -euo pipefail

root="${1:?Usage: check_glibc_floor.sh <directory> [GLIBC_2.28]}"
floor="${2:-GLIBC_2.28}"
[[ -d $root ]] || { echo "Missing binary directory: $root" >&2; exit 1; }
[[ $floor =~ ^GLIBC_[0-9]+\.[0-9]+$ ]] || { echo "Invalid glibc floor: $floor" >&2; exit 1; }

status=0
count=0
while IFS= read -r -d '' elf; do
    count=$((count + 1))
    if ! symbols="$(objdump -T "$elf")"; then
        echo "Cannot inspect $elf" >&2
        status=1
        continue
    fi
    need="$(printf '%s\n' "$symbols" | grep -o 'GLIBC_[0-9][0-9.]*' | sort -Vu | tail -n 1 || true)"
    [[ -n $need ]] || continue
    if [[ $(printf '%s\n%s\n' "$need" "$floor" | sort -V | tail -n 1) != "$floor" ]]; then
        echo "$elf needs $need (maximum $floor)" >&2
        status=1
    fi
done < <(find "$root" -type f \( -name ouasim -o -name '*.so*' \) -print0)
[[ $count -gt 0 ]] || { echo "No binaries found in $root" >&2; exit 1; }
exit "$status"
