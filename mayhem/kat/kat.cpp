// Known-answer probe for the CGAL functional oracle (mayhem/test.sh). Each line it prints is
// `<case>: <value>`, and test.sh diffs every line against mayhem/kat/expected.txt. The values are
// the mathematically known answers for the given inputs (cube volume, Delaunay of a square, ...).
// Built with the project's NORMAL flags, dynamically linked, reads fixtures from the upstream
// Data/ directory (argv[1] = repo root).
#define CGAL_NO_CDT_2_WARNING
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/IO/polygon_mesh_io.h>
#include <CGAL/IO/polygon_soup_io.h>
#include <CGAL/Polygon_mesh_processing/measure.h>
#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Polygon_mesh_processing/transform.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Delaunay_triangulation_3.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/convex_hull_3.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Search_traits_3.h>
#include <CGAL/Kd_tree.h>
#include <CGAL/Orthogonal_k_neighbor_search.h>
#include <CGAL/Fuzzy_sphere.h>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <string>

namespace PMP = CGAL::Polygon_mesh_processing;
typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Exact_predicates_exact_constructions_kernel EK;

static std::string root;
static std::string data(const char* rel) { return root + "/Data/data/" + rel; }

static void soup_counts(const char* name, const char* rel) {
  std::vector<K::Point_3> pts;
  std::vector<std::vector<std::size_t>> polys;
  bool ok = CGAL::IO::read_polygon_soup(data(rel), pts, polys);
  std::cout << name << ": ok=" << ok << " points=" << pts.size() << " polygons=" << polys.size() << "\n";
}

int main(int argc, char** argv) {
  if (argc < 2) { std::cerr << "usage: kat <repo-root>\n"; return 2; }
  root = argv[1];
  std::cout << std::setprecision(12);

  {  // mesh I/O + measures
    CGAL::Surface_mesh<K::Point_3> m;
    bool ok = CGAL::IO::read_polygon_mesh(data("meshes/cube.off"), m);
    std::cout << "cube_off: ok=" << ok << " vertices=" << num_vertices(m) << " faces=" << num_faces(m)
              << " closed=" << CGAL::is_closed(m) << " volume=" << PMP::volume(m)
              << " area=" << PMP::area(m) << "\n";
  }
  {
    CGAL::Polyhedron_3<K> p;
    std::ifstream in(data("meshes/tetrahedron.off"));
    in >> p;
    std::cout << "tetrahedron_polyhedron: vertices=" << p.size_of_vertices()
              << " facets=" << p.size_of_facets() << " closed=" << p.is_closed()
              << " abs_volume=" << std::fabs(PMP::volume(p)) << "\n";
  }
  soup_counts("sphere_ply_soup", "meshes/sphere.ply");
  soup_counts("sphere_stl_soup", "meshes/sphere.stl");
  soup_counts("pig_obj_soup", "meshes/pig.obj");

  {  // exact predicates
    K::Point_2 a(0, 0), b(1, 1), c(2, 2.0000000000000004), d(2, 1.9999999999999998);
    std::cout << "orientation_above_line: " << static_cast<int>(CGAL::orientation(a, b, c)) << "\n";
    std::cout << "orientation_below_line: " << static_cast<int>(CGAL::orientation(a, b, d)) << "\n";
    std::cout << "orientation_on_line: " << static_cast<int>(CGAL::orientation(a, b, K::Point_2(3, 3))) << "\n";
  }
  {  // polygon
    typedef CGAL::Polygon_2<K> Poly;
    Poly sq;
    sq.push_back(K::Point_2(0, 0)); sq.push_back(K::Point_2(2, 0));
    sq.push_back(K::Point_2(2, 2)); sq.push_back(K::Point_2(0, 2));
    std::cout << "square_polygon: area=" << sq.area() << " convex=" << sq.is_convex()
              << " simple=" << sq.is_simple() << " orientation=" << static_cast<int>(sq.orientation()) << "\n";
  }
  {  // 2D triangulations
    CGAL::Delaunay_triangulation_2<K> dt;
    dt.insert(K::Point_2(0, 0)); dt.insert(K::Point_2(2, 0)); dt.insert(K::Point_2(2, 2));
    dt.insert(K::Point_2(0, 2)); dt.insert(K::Point_2(1, 1));
    std::cout << "delaunay2_square_center: vertices=" << dt.number_of_vertices()
              << " faces=" << dt.number_of_faces() << " valid=" << dt.is_valid() << "\n";

    typedef CGAL::Constrained_Delaunay_triangulation_2<EK, CGAL::Default, CGAL::Exact_intersections_tag> CDT;
    CDT cdt;
    cdt.insert_constraint(EK::Point_2(0, 0), EK::Point_2(2, 2));
    cdt.insert_constraint(EK::Point_2(2, 0), EK::Point_2(0, 2));
    std::size_t constrained = 0;
    for (auto e = cdt.finite_edges_begin(); e != cdt.finite_edges_end(); ++e)
      if (cdt.is_constrained(*e)) ++constrained;
    std::cout << "cdt2_crossing_diagonals: vertices=" << cdt.number_of_vertices()
              << " constrained_edges=" << constrained << " faces=" << cdt.number_of_faces()
              << " valid=" << cdt.is_valid() << "\n";
  }
  {  // 3D triangulation + hull
    CGAL::Delaunay_triangulation_3<K> dt;
    dt.insert(K::Point_3(0, 0, 0)); dt.insert(K::Point_3(1, 0, 0));
    dt.insert(K::Point_3(0, 1, 0)); dt.insert(K::Point_3(0, 0, 1));
    std::cout << "delaunay3_tetrahedron: vertices=" << dt.number_of_vertices()
              << " cells=" << dt.number_of_finite_cells() << " dimension=" << dt.dimension()
              << " valid=" << dt.is_valid() << "\n";

    std::vector<K::Point_3> pts;
    for (int x = -1; x <= 1; x += 2)
      for (int y = -1; y <= 1; y += 2)
        for (int z = -1; z <= 1; z += 2) pts.emplace_back(x, y, z);
    pts.emplace_back(0, 0, 0); pts.emplace_back(0.5, 0.25, -0.5);
    CGAL::Surface_mesh<K::Point_3> hull;
    CGAL::convex_hull_3(pts.begin(), pts.end(), hull);
    std::cout << "convex_hull3_cube_with_interior: vertices=" << num_vertices(hull)
              << " faces=" << num_faces(hull) << " volume=" << PMP::volume(hull) << "\n";
  }
  {  // spatial searching
    typedef CGAL::Search_traits_3<K> Traits;
    typedef CGAL::Kd_tree<Traits> Tree;
    typedef CGAL::Orthogonal_k_neighbor_search<Traits> Knn;
    std::vector<K::Point_3> pts;
    for (int x = -1; x <= 1; x += 2)
      for (int y = -1; y <= 1; y += 2)
        for (int z = -1; z <= 1; z += 2) pts.emplace_back(x, y, z);
    pts.emplace_back(0, 0, 0);
    Tree tree(pts.begin(), pts.end());
    K::Point_3 q(0.1, 0.2, 0.3);
    Knn knn(tree, q, 2);
    std::cout << "kdtree_two_nearest:";
    for (auto it = knn.begin(); it != knn.end(); ++it) std::cout << " " << it->second;
    std::cout << " nearest_point=" << knn.begin()->first << "\n";
    std::list<K::Point_3> in_sphere;
    tree.search(std::back_inserter(in_sphere), CGAL::Fuzzy_sphere<Traits>(K::Point_3(0, 0, 0), 1.5, 0.0));
    std::cout << "kdtree_sphere_r1.5: " << in_sphere.size() << "\n";
    in_sphere.clear();
    tree.search(std::back_inserter(in_sphere), CGAL::Fuzzy_sphere<Traits>(K::Point_3(0, 0, 0), 1.8, 0.0));
    std::cout << "kdtree_sphere_r1.8: " << in_sphere.size() << "\n";
  }
  {  // exact corefinement: unit-offset union of two [-1,1]^3 cubes has volume 8 + 8 - 4
    typedef CGAL::Surface_mesh<EK::Point_3> EMesh;
    EMesh a, b, u;
    CGAL::IO::read_polygon_mesh(data("meshes/cube.off"), a);
    CGAL::IO::read_polygon_mesh(data("meshes/cube.off"), b);
    PMP::transform(CGAL::Aff_transformation_3<EK>(CGAL::TRANSLATION, EK::Vector_3(1, 0, 0)), b);
    bool ok = PMP::corefine_and_compute_union(a, b, u);
    std::cout << "corefine_union_cubes: ok=" << ok << " volume=" << CGAL::to_double(PMP::volume(u))
              << " closed=" << CGAL::is_closed(u) << "\n";
  }
  return 0;
}
