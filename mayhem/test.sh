#!/usr/bin/env bash
#
# mayhem/test.sh — RUN the CGAL functional oracle that mayhem/build.sh produced ($SRC/cgal-kat).
# The probe exercises real CGAL algorithms (mesh I/O, exact predicates, Delaunay / constrained
# Delaunay triangulations, convex hull, kd-tree search, exact corefinement) on fixtures from the
# upstream Data/ directory and prints `<case>: <value>` lines; every line of
# mayhem/kat/expected.txt (mathematically known answers) must appear in that output verbatim.
# The comparison runs here in bash, so a neutered probe (empty output) fails every case.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
cd "$SRC"

emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$skipped" "$pending" "$other"
  [ "$failed" -eq 0 ]
}

probe="$SRC/cgal-kat"
expected="$SRC/mayhem/kat/expected.txt"
[ -x "$probe" ] || { echo "test.sh: $probe missing — mayhem/build.sh should have built it" >&2; emit_ctrf cgal-kat 0 1; exit 1; }
[ -s "$expected" ] || { echo "test.sh: $expected missing" >&2; emit_ctrf cgal-kat 0 1; exit 1; }

actual="$("$probe" "$SRC" 2>/dev/null)"
passed=0; failed=0
while IFS= read -r want; do
  [ -n "$want" ] || continue
  if printf '%s\n' "$actual" | grep -qxF -- "$want"; then
    passed=$((passed + 1))
  else
    failed=$((failed + 1))
    echo "FAIL: expected line not produced: $want" >&2
    echo "      probe line:   $(printf '%s\n' "$actual" | grep -F -- "${want%%:*}:" | head -1)" >&2
  fi
done < "$expected"
emit_ctrf cgal-kat "$passed" "$failed"
