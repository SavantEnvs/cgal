// 3D Delaunay triangulation: insert, locate, nearest vertex, removal, validity.
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Delaunay_triangulation_3.h>
#include "common.h"

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Delaunay_triangulation_3<K> DT;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  ByteReader r(data, size);
  uint8_t mode, count;
  if (!r.u8(mode) || !r.u8(count)) return -1;
  const size_t n = 4 + count % 124;
  std::vector<double> xyz;
  if (!read_points<3>(r, xyz, n)) return -1;

  DT dt;
  for (size_t i = 0; i < n; ++i)
    dt.insert(K::Point_3(xyz[3 * i], xyz[3 * i + 1], xyz[3 * i + 2]));
  if (!dt.is_valid()) __builtin_trap();
  K::Point_3 q((xyz[0] + xyz[3]) * 0.5, (xyz[1] + xyz[4]) * 0.5, (xyz[2] + xyz[5]) * 0.5);
  DT::Locate_type lt; int li, lj;
  dt.locate(q, lt, li, lj);
  if (dt.number_of_vertices() > 0) dt.nearest_vertex(q);
  size_t k = 0;
  std::vector<DT::Vertex_handle> victims;
  for (auto v = dt.finite_vertices_begin(); v != dt.finite_vertices_end(); ++v)
    if ((k++ % 4) == (mode % 4)) victims.push_back(v);
  for (auto v : victims) dt.remove(v);
  if (!dt.is_valid()) __builtin_trap();
  return 0;
}
