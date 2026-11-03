// Shared helpers for the CGAL libFuzzer harnesses. Harnesses take bytes only from the fuzzer
// (no file I/O) and reject unusable input with -1.
#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>
#include <CGAL/assertions_behaviour.h>

// A failed CGAL_assertion/precondition/postcondition aborts (real CWE, first application frame)
// instead of throwing; throwing from ~Polyhedron_incremental_builder_3 would std::terminate.
// Not inline: libFuzzer only references it weakly, so an unused inline definition is never emitted.
extern "C" int LLVMFuzzerInitialize(int*, char***) {
  CGAL::set_error_behaviour(CGAL::ABORT);
  return 0;
}

// Sequential reader over the fuzzer's bytes.
struct ByteReader {
  const uint8_t* p;
  size_t n;
  ByteReader(const uint8_t* d, size_t s) : p(d), n(s) {}
  size_t left() const { return n; }
  bool u8(uint8_t& v) {
    if (n < 1) return false;
    v = *p++; --n; return true;
  }
  // float32 widened to double; false when the bytes are missing or the value is not finite.
  // Finite only: CGAL's number domain is the reals (Number_types/doc/Number_types/CGAL/double.h:5-18:
  // double models RealEmbeddable, is_finite = "a value in R"; Algebraic_foundations/doc/.../Concepts/
  // RealEmbeddable.h:6-8: "total order of the real numbers") and it asserts finiteness at every
  // double -> filter/exact boundary: Interval_nt.h:134, GMP/Gmpq_type.h:164, Mpzf.h:447
  // (CGAL_assertion(is_finite(d)) / dexp != 2047). No \pre states it, so NaN/inf are not a bug report.
  bool coord(double& v) {
    if (n < sizeof(float)) return false;
    float f;
    std::memcpy(&f, p, sizeof f);
    p += sizeof f; n -= sizeof f;
    if (!std::isfinite(f)) return false;
    v = f;
    return true;
  }
};

// Exactly `count` points of dimension D as consecutive float32 coordinates, flattened into `out`.
// False when the bytes run out or any coordinate is not finite.
template <int D>
inline bool read_points(ByteReader& r, std::vector<double>& out, size_t count) {
  for (size_t c = 0; c < count; ++c)
    for (int i = 0; i < D; ++i) {
      double v;
      if (!r.coord(v)) return false;
      out.push_back(v);
    }
  return true;
}
