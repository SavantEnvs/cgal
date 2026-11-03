// 2D Delaunay triangulation (insert / locate / nearest / remove) and a constrained Delaunay
// triangulation with exact intersections of constraint segments.
#define CGAL_NO_CDT_2_WARNING
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include "common.h"

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Delaunay_triangulation_2<K> DT;
typedef CGAL::Exact_predicates_exact_constructions_kernel EK;
typedef CGAL::Exact_intersections_tag Itag;
typedef CGAL::Constrained_Delaunay_triangulation_2<EK, CGAL::Default, Itag> CDT;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  ByteReader r(data, size);
  uint8_t mode, count;
  if (!r.u8(mode) || !r.u8(count)) return -1;
  const size_t n = 3 + count % 126;
  std::vector<double> xy;
  if (!read_points<2>(r, xy, n)) return -1;

  DT dt;
  for (size_t i = 0; i < n; ++i) dt.insert(K::Point_2(xy[2 * i], xy[2 * i + 1]));
  if (!dt.is_valid()) __builtin_trap();
  K::Point_2 q(xy[0] * 0.5 + xy[2] * 0.5, xy[1] * 0.5 + xy[3] * 0.5);
  DT::Locate_type lt; int li;
  dt.locate(q, lt, li);
  if (dt.number_of_vertices() > 0) dt.nearest_vertex(q);
  size_t k = 0;
  std::vector<DT::Vertex_handle> victims;
  for (auto v = dt.finite_vertices_begin(); v != dt.finite_vertices_end(); ++v)
    if ((k++ % 3) == (mode % 3)) victims.push_back(v);
  for (auto v : victims) dt.remove(v);
  if (!dt.is_valid()) __builtin_trap();

  CDT cdt;
  std::vector<CDT::Vertex_handle> vh;
  const size_t m = n;
  for (size_t i = 0; i < m; ++i) vh.push_back(cdt.insert(EK::Point_2(xy[2 * i], xy[2 * i + 1])));
  uint8_t a, b;
  while (r.u8(a) && r.u8(b)) {
    auto va = vh[a % vh.size()], vb = vh[b % vh.size()];
    if (va != vb) cdt.insert_constraint(va, vb);
  }
  if (!cdt.is_valid()) __builtin_trap();
  return 0;
}
