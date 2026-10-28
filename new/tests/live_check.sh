#!/usr/bin/env bash
# Live-app phase check (P2.9 / P3.9): drives the REAL ncurses editor through a
# pty with a scripted key sequence and fails on a sanitizer report or a
# non-zero exit. Not part of ctest (it needs a terminal).
#
#   tests/live_check.sh "<keys>" [file]      # run from the build directory
#
# Keys are plain text (printf-style isn't expanded, so use $'\x04' etc. for
# control keys). Without a file a 40-line sample is generated.
set -uo pipefail

keys=${1:-}
file=${2:-}
bin=${BIN:-./noni}

if [[ ! -x $bin ]]; then
    echo "FAIL: $bin not found (run from the build directory)" >&2
    exit 1
fi

work=$(mktemp -d "${TMPDIR:-/tmp}/noni-live.XXXXXX") || exit 1
trap 'rm -rf "$work"' EXIT

if [[ -z $file ]]; then
    file=$work/sample.txt
    {
        printf 'alpha beta gamma (one) [two] {three}\n'
        printf '    call(x(1), [2, 3]);\n'
        printf '}\n'
        printf '\n'
        for i in $(seq 1 36); do
            if (( i % 9 == 0 )); then
                printf '\n'
            else
                printf 'line %02d with a few words and (parens)\n' "$i"
            fi
        done
    } >"$file"
fi

screen=$work/screen.log
# shellcheck disable=SC2016
( sleep 0.4; printf '%s' "$keys"; sleep 0.3 ) \
    | timeout "${TIMEOUT:-10}" script -qec "ASAN_OPTIONS=detect_leaks=1 $bin '$file'" /dev/null \
        >"$screen" 2>&1
status=$?

if (( status == 124 )); then
    echo "FAIL: the key script did not quit the editor (end it with 'q')" >&2
    exit 1
fi

if (( status != 0 )); then
    echo "FAIL: editor exited with status $status" >&2
    tail -c 2000 "$screen" >&2
    exit 1
fi

if grep -qE 'Sanitizer|LeakSanitizer|runtime error:' "$screen"; then
    echo "FAIL: sanitizer report" >&2
    grep -nE 'Sanitizer|LeakSanitizer|runtime error:' "$screen" >&2
    exit 1
fi

if [[ ! -s $screen ]]; then
    echo "FAIL: no screen output (the editor never drew)" >&2
    exit 1
fi

echo "OK: clean exit, no sanitizer findings ($(wc -c <"$screen") bytes drawn)"
