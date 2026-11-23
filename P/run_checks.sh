#!/usr/bin/env bash
# Compile the model and run every test case. tcPass* must find no bug; tcFail* must find one
# (each tcFail* is a finding against the current firmware, see README.md).
set -u
cd "$(dirname "$0")"
SCHED=${SCHED:-5000}
p compile >/dev/null || { echo "compile failed"; exit 1; }
rc=0
for t in $(grep -oE 'test tc[A-Za-z]+' PTst/TestScripts.p | awk '{print $2}'); do
  out=$(p check -tc "$t" -s "$SCHED" 2>&1)
  # 'p check -tc X' runs every test whose name starts with X: take the first result block only
  bugs=$(echo "$out" | grep -oE 'Found [0-9]+ bug' | head -1 | awk '{print $2}')
  case "$t" in
    tcPass*) [ "$bugs" = 0 ] && s=ok || { s=UNEXPECTED; rc=1; } ;;
    tcFail*) [ "${bugs:-0}" != 0 ] && s=ok || { s=UNEXPECTED; rc=1; } ;;
  esac
  printf '%-34s bugs=%-3s %s\n' "$t" "${bugs:-?}" "$s"
done
exit $rc
