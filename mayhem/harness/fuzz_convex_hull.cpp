// Convex hulls: 3D (Surface_mesh output) and the 2D family of algorithms on the xy projection.
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/convex_hull_3.h>
#include <CGAL/convex_hull_2.h>
#include <CGAL/ch_jarvis.h>
#include <CGAL/ch_eddy.h>
#include <CGAL/ch_bykat.h>
#include <CGAL/ch_graham_andrew.h>
#include <CGAL/ch_akl_toussaint.h>
#include <CGAL/Polygon_mesh_processing/measure.h>
#include "common.h"

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef K::Point_2 Point_2;
typedef K::Point_3 Point_3;
typedef CGAL::Surface_mesh<Point_3> Mesh;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  ByteReader r(data, size);
  uint8_t count;
  if (!r.u8(count)) return -1;
  const size_t n = 3 + count % 125;
  std::vector<double> xyz;
  if (!read_points<3>(r, xyz, n)) return -1;

  std::vector<Point_3> p3;
  std::vector<Point_2> p2;
  for (size_t i = 0; i < n; ++i) {
    p3.emplace_back(xyz[3 * i], xyz[3 * i + 1], xyz[3 * i + 2]);
    p2.emplace_back(xyz[3 * i], xyz[3 * i + 1]);
  }

  std::vector<Point_2> h;
  CGAL::convex_hull_2(p2.begin(), p2.end(), std::back_inserter(h));
  h.clear(); CGAL::ch_graham_andrew(p2.begin(), p2.end(), std::back_inserter(h));
  h.clear(); CGAL::ch_jarvis(p2.begin(), p2.end(), std::back_inserter(h));
  h.clear(); CGAL::ch_bykat(p2.begin(), p2.end(), std::back_inserter(h));
  h.clear(); CGAL::ch_eddy(p2.begin(), p2.end(), std::back_inserter(h));
  h.clear(); CGAL::ch_akl_toussaint(p2.begin(), p2.end(), std::back_inserter(h));

  // Degenerate (collinear / coplanar) input legitimately yields a flat, non-strongly-convex hull.
  bool full_dim = false;
  {
    size_t b = 1, c = 2;
    while (c < n && CGAL::collinear(p3[0], p3[b], p3[c])) ++c;
    if (c < n)
      for (size_t i = c + 1; i < n && !full_dim; ++i) full_dim = !CGAL::coplanar(p3[0], p3[b], p3[c], p3[i]);
  }

  Mesh hull;
  CGAL::convex_hull_3(p3.begin(), p3.end(), hull);
  if (full_dim && CGAL::is_triangle_mesh(hull)) {
    volatile bool convex = CGAL::is_strongly_convex_3(hull);
    (void)convex;
    volatile double vol = CGAL::Polygon_mesh_processing::volume(hull);
    (void)vol;
  }
  return 0;
}
