// Spatial searching over a Kd_tree of 3D points: fuzzy sphere / iso-box range queries,
// k-nearest neighbours and incremental nearest-neighbour iteration.
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Search_traits_3.h>
#include <CGAL/Kd_tree.h>
#include <CGAL/Fuzzy_sphere.h>
#include <CGAL/Fuzzy_iso_box.h>
#include <CGAL/Orthogonal_k_neighbor_search.h>
#include <CGAL/Orthogonal_incremental_neighbor_search.h>
#include <iterator>
#include <list>
#include "common.h"

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef K::Point_3 Point_3;
typedef CGAL::Search_traits_3<K> Traits;
typedef CGAL::Kd_tree<Traits> Tree;
typedef CGAL::Fuzzy_sphere<Traits> Fuzzy_sphere;
typedef CGAL::Fuzzy_iso_box<Traits> Fuzzy_box;
typedef CGAL::Orthogonal_k_neighbor_search<Traits> Knn;
typedef CGAL::Orthogonal_incremental_neighbor_search<Traits> Inc;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  ByteReader r(data, size);
  uint8_t count, k;
  if (!r.u8(count) || !r.u8(k)) return -1;
  const size_t n = 1 + count % 127;
  std::vector<double> xyz;
  if (!read_points<3>(r, xyz, n)) return -1;
  double q[7];
  for (double& v : q)
    if (!r.coord(v)) return -1;

  std::vector<Point_3> pts;
  for (size_t i = 0; i < n; ++i) pts.emplace_back(xyz[3 * i], xyz[3 * i + 1], xyz[3 * i + 2]);
  Tree tree(pts.begin(), pts.end());
  tree.build();

  Point_3 c(q[0], q[1], q[2]);
  double radius = std::fabs(q[3]), eps = std::fabs(q[4]) * 0.01;
  std::list<Point_3> out;
  tree.search(std::back_inserter(out), Fuzzy_sphere(c, radius, eps));
  out.clear();
  Point_3 a(q[0], q[1], q[2]), b(q[5], q[6], q[3]);
  tree.search(std::back_inserter(out), Fuzzy_box(a, b, eps));

  Knn knn(tree, c, 1 + k % 16);
  for (auto it = knn.begin(); it != knn.end(); ++it) (void)it->second;
  Inc inc(tree, c);
  int taken = 0;
  for (auto it = inc.begin(); it != inc.end() && taken < 32; ++it, ++taken) (void)it->second;
  return 0;
}
