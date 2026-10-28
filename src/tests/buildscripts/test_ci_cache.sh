#!/usr/bin/env bash
set -euo pipefail

fail() {
    echo "$*" >&2
    exit 1
}

test_dir="$(mktemp -d)"
trap 'rm -rf "$test_dir"' EXIT
export CACHE_TEST_LOG="$test_dir/commands"
export CACHE_TEST_ENTRIES="$test_dir/entries"
export GITHUB_REF=refs/heads/main
export PREFIX=ccache-build-test-ubuntu-latest-6.9.3-
export KEEP="${PREFIX}new"

sed -n '/^      run: |$/,$ { /^      run: |$/d; s/^        //; p; }' \
    .github/actions/drop-stale-caches/action.yml > "$test_dir/cleanup.sh"
test -s "$test_dir/cleanup.sh" || fail 'Missing cache cleanup script'

cat > "$test_dir/gh" <<'MOCK'
#!/usr/bin/env bash
set -euo pipefail
if [ "$1 $2" = 'cache list' ]; then
    [ "$3" = --ref ] && [ "$4" = "$GITHUB_REF" ] || exit 21
    [ "$5" = --key ] && [ "$6" = "$PREFIX" ] || exit 22
    if [ "${FAIL_STAGE:-}" = list ]; then exit 7; fi
    cat "$CACHE_TEST_ENTRIES"
elif [ "$1 $2" = 'cache delete' ]; then
    if [ "${FAIL_STAGE:-}" = delete ]; then exit 8; fi
    printf '%s\n' "$3" >> "$CACHE_TEST_LOG"
else
    exit 23
fi
MOCK
chmod +x "$test_dir/gh"
export PATH="$test_dir:$PATH"
unset FAIL_STAGE

printf '101 %sold\n102 %snew\n103 %solder\n' "$PREFIX" "$PREFIX" "$PREFIX" > "$CACHE_TEST_ENTRIES"
: > "$CACHE_TEST_LOG"
bash -e -o pipefail "$test_dir/cleanup.sh"
printf '101\n103\n' > "$test_dir/expected"
cmp "$test_dir/expected" "$CACHE_TEST_LOG" || fail 'Cleanup must preserve the requested key'

: > "$CACHE_TEST_LOG"
KEEP='' bash -e -o pipefail "$test_dir/cleanup.sh"
printf '101\n102\n103\n' > "$test_dir/expected"
cmp "$test_dir/expected" "$CACHE_TEST_LOG" || fail 'Cleanup must delete all superseded entries'

for stage in list delete; do
    : > "$CACHE_TEST_LOG"
    if FAIL_STAGE="$stage" bash -e -o pipefail "$test_dir/cleanup.sh"; then
        fail "Cache $stage failure was ignored"
    fi
    test ! -s "$CACHE_TEST_LOG" || fail "Cleanup continued after a $stage failure"
done

: > "$CACHE_TEST_ENTRIES"
: > "$CACHE_TEST_LOG"
bash -e -o pipefail "$test_dir/cleanup.sh"
test ! -s "$CACHE_TEST_LOG" || fail 'Empty cache must not trigger deletions'
