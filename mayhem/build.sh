#!/usr/bin/env bash
#
# mayhem/build.sh — build the CGAL libFuzzer harnesses (+ standalone reproducers) and the
# known-answer probe that mayhem/test.sh runs. CGAL is a header-only library, so "building the
# project" means compiling the instrumented library templates into each harness with
# $SANITIZER_FLAGS $DEBUG_FLAGS. Idempotent and offline: it needs only the in-image headers
# (libboost-dev, libgmp-dev, libmpfr-dev from the Dockerfile) and the checked-out tree.
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"
mkdir -p build/obj

# CGAL's headers live in <Package>/include for every package of the source tree.
INC=()
for d in "$SRC"/*/include; do INC+=("-I$d"); done

# float-cast-overflow is relaxed only for the two File_scanner_OFF targets (fuzz_off, fuzz_polyhedron_kd),
# via the extra-flags column below: the OFF scanner casts parsed doubles to size_t/uchar. Every other target
# keeps it halting; ASan and every other UBSan check stay halting everywhere.
# -O1 keeps the exact-arithmetic-heavy harnesses fast enough to fuzz. CGAL's assertions, preconditions and
# postconditions stay compiled in (no -DNDEBUG); harness/common.h's LLVMFuzzerInitialize makes a failed one
# abort (CGAL::set_error_behaviour(ABORT)) instead of throwing.
CXXSTD=(-std=c++20 -O1)
LIBS=(-lgmp -lmpfr)

# Standalone (run-once, non-libFuzzer) driver: compiled as C so its LLVMFuzzerTestOneInput
# reference keeps C linkage.
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o build/obj/standalone_main.o
# Leak sanitization off at build time (ASan and UBSan stay fully active).
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c "$SRC/mayhem/lsan_off.cc" -o build/obj/lsan_off.o

# <binary> <harness source> [extra compile flags...]
TARGETS=(
  "fuzz_off|fuzz_soup.cpp|-DFMT_OFF -fno-sanitize=float-cast-overflow"
  "fuzz_obj|fuzz_soup.cpp|-DFMT_OBJ"
  "fuzz_ply|fuzz_soup.cpp|-DFMT_PLY"
  "fuzz_stl|fuzz_soup.cpp|-DFMT_STL"
  "fuzz_delaunay_2|fuzz_delaunay_2.cpp|"
  "fuzz_delaunay_3|fuzz_delaunay_3.cpp|"
  "fuzz_convex_hull|fuzz_convex_hull.cpp|"
  "fuzz_kd_tree|fuzz_kd_tree.cpp|"
  "fuzz_polyhedron_kd|fuzz_polyhedron_kd.cpp|-fno-sanitize=float-cast-overflow"
)

fail=0
compile_one() {
  local name="$1" src="$2" defs="$3"
  # shellcheck disable=SC2086
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS -fsanitize=fuzzer-no-link "${CXXSTD[@]}" $defs "${INC[@]}" \
      -c "$SRC/mayhem/harness/$src" -o "build/obj/$name.o"
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE "build/obj/$name.o" build/obj/lsan_off.o \
      "${LIBS[@]}" -o "/mayhem/$name"
  $CXX $SANITIZER_FLAGS $DEBUG_FLAGS "build/obj/$name.o" build/obj/standalone_main.o build/obj/lsan_off.o \
      "${LIBS[@]}" -o "/mayhem/$name-standalone"
}

# The functional-oracle probe: the project's NORMAL flags (no sanitizers), dynamically linked.
build_kat() {
  # shellcheck disable=SC2086
  env -u CXXFLAGS -u CFLAGS -u LDFLAGS $CXX -std=c++20 -O0 $COVERAGE_FLAGS "${INC[@]}" \
      "$SRC/mayhem/kat/kat.cpp" "${LIBS[@]}" -o "$SRC/cgal-kat"
}

pids=()
spawn() {
  while [ "$(jobs -rp | wc -l)" -ge "$MAYHEM_JOBS" ]; do wait -n || fail=1; done
  "$@" & pids+=($!)
}
for t in "${TARGETS[@]}"; do
  IFS='|' read -r name src defs <<<"$t"
  spawn compile_one "$name" "$src" "$defs"
done
spawn build_kat
for p in "${pids[@]}"; do wait "$p" || fail=1; done
[ "$fail" -eq 0 ] || { echo "build.sh: a harness build failed" >&2; exit 1; }

for t in "${TARGETS[@]}"; do
  IFS='|' read -r name _ _ <<<"$t"
  [ -x "/mayhem/$name" ] && [ -x "/mayhem/$name-standalone" ] || { echo "missing /mayhem/$name" >&2; exit 1; }
done
[ -x "$SRC/cgal-kat" ] || { echo "missing $SRC/cgal-kat" >&2; exit 1; }
echo "build.sh: ok"
