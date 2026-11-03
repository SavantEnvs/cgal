// Polygon-soup readers (OFF / OBJ / PLY / STL, selected at compile time) followed by the standard
// soup -> repair -> orient -> Surface_mesh -> triangulate -> measure pipeline, following
// PMP::IO::read_polygon_mesh (no input filtering beyond what it does).
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/IO/OFF.h>
#include <CGAL/IO/OBJ.h>
#include <CGAL/IO/PLY.h>
#include <CGAL/IO/STL.h>
#include <CGAL/Polygon_mesh_processing/repair_polygon_soup.h>
#include <CGAL/Polygon_mesh_processing/orient_polygon_soup.h>
#include <CGAL/Polygon_mesh_processing/polygon_soup_to_polygon_mesh.h>
#include <CGAL/Polygon_mesh_processing/triangulate_faces.h>
#include <CGAL/Polygon_mesh_processing/measure.h>
#include <CGAL/Polygon_mesh_processing/self_intersections.h>
#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <sstream>
#include <string>
#include "common.h"

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef K::Point_3 Point_3;
typedef CGAL::Surface_mesh<Point_3> Mesh;
namespace PMP = CGAL::Polygon_mesh_processing;

#if defined(FMT_OFF)
#  define READ(is, pts, polys) CGAL::IO::read_OFF(is, pts, polys)
#elif defined(FMT_OBJ)
#  define READ(is, pts, polys) CGAL::IO::read_OBJ(is, pts, polys)
#elif defined(FMT_PLY)
#  define READ(is, pts, polys) CGAL::IO::read_PLY(is, pts, polys)
#elif defined(FMT_STL)
#  define READ(is, pts, polys) CGAL::IO::read_STL(is, pts, polys)
#else
#  error "define one of FMT_OFF FMT_OBJ FMT_PLY FMT_STL"
#endif

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  std::istringstream in(std::string(reinterpret_cast<const char*>(data), size));
  std::vector<Point_3> points;
  std::vector<std::vector<std::size_t>> polygons;
  if (!READ(in, points, polygons)) return -1;

  PMP::repair_polygon_soup(points, polygons);
  PMP::orient_polygon_soup(points, polygons);
  // Same gate as PMP::IO::read_polygon_mesh (polygon_mesh_io.h): polygon_soup_to_polygon_mesh requires it.
  if (!PMP::is_polygon_soup_a_polygon_mesh(polygons)) return 0;
  Mesh mesh;
  PMP::polygon_soup_to_polygon_mesh(points, polygons, mesh);
  if (num_faces(mesh) == 0) return 0;

  PMP::triangulate_faces(mesh);
  volatile double area = PMP::area(mesh);
  (void)area;
  volatile bool si = PMP::does_self_intersect(mesh);
  (void)si;
  // PMP::is_outward_oriented documents `@param tm the closed triangle mesh free from self-intersections`
  // (PMP_Mesh_repair/.../orientation_basic.h:154; its assertion at :109 is "no self-intersection"), and
  // volume() takes a closed triangle mesh, so both run only when that precondition holds.
  if (CGAL::is_closed(mesh) && CGAL::is_triangle_mesh(mesh) && !si) {
    volatile bool out = PMP::is_outward_oriented(mesh);
    (void)out;
    volatile double vol = PMP::volume(mesh);
    (void)vol;
  }
  return 0;
}
