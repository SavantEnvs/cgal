// Port of upstream's example Spatial_searching/searching_polyhedron_vertices_with_fuzzy_sphere.cpp:
// read a Polyhedron_3 from an OFF stream, index its vertices in a Kd_tree and run fuzzy-sphere
// searches. The mesh comes from the fuzzer bytes instead of a file.
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/Search_traits_3.h>
#include <CGAL/Search_traits_adapter.h>
#include <CGAL/Kd_tree.h>
#include <CGAL/Fuzzy_sphere.h>
#include <iterator>
#include <list>
#include <sstream>
#include <string>
#include "common.h"

typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
typedef Kernel::Point_3 Point_3;
typedef CGAL::Polyhedron_3<Kernel> Mesh;
typedef boost::graph_traits<Mesh>::vertex_descriptor vertex_descriptor;
typedef boost::property_map<Mesh, CGAL::vertex_point_t>::type Vertex_point_pmap;
typedef CGAL::Search_traits_3<Kernel> Traits_base;
typedef CGAL::Search_traits_adapter<vertex_descriptor, Vertex_point_pmap, Traits_base> Traits;
typedef CGAL::Kd_tree<Traits> Tree;
typedef Tree::Splitter Splitter;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  std::istringstream in(std::string(reinterpret_cast<const char*>(data), size));
  Mesh mesh;
  in >> mesh;
  if (!in || mesh.empty()) return -1;

  Vertex_point_pmap vppmap = get(CGAL::vertex_point, mesh);
  Traits traits(vppmap);
  Tree tree(vertices(mesh).begin(), vertices(mesh).end(), Splitter(), traits);

  const Point_3 queries[2] = {Point_3(0.0, 0.0, 0.0), get(vppmap, *vertices(mesh).begin())};
  for (const Point_3& query : queries) {
    CGAL::Fuzzy_sphere<Traits> fz(query, 0.5, 0.01, traits);
    std::list<vertex_descriptor> result;
    tree.search(std::back_inserter(result), fz);
  }
  return 0;
}
