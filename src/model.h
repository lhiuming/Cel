#ifndef CEL_MODEL_H
#define CEL_MODEL_H

#include <vector>
#include <memory>

#include "algebra.h"
#include "color.h"

/*
 * model.h
 * This module considers how a model is composed.
 *
 * NOTE: A Model object shoule be unique, but possible to have multiple
 * "instances" by relating with different transforms. (see scene.h)
 *
 * TODO: add aggregated model (and animated model)
 * TODO: support move constructors
 * TODO: Support cube and sphere
 */

namespace CEL {


// Model classes //////////////////////////////////////////////////////////////

// Base class
// supports polymophism only through RTTI
class Model {
public:

  virtual ~Model() = default;

};


// Trianglular Mesh
class Mesh : public Model {
public:

  // Simple Vertex class
  struct Vertex {
    Vec4 coord; // Vertex position in right-hand world space
    Vec3 normal; // Vertex normal
    Color color; // Vertec color

    Vertex(const Vec3& pos3, const Color& c = Color(0.5, 0.5, 0.5, 1.0))
      : coord(pos3, 1.0), normal(), color(c) {};
    Vertex(const Vec3& pos3, const Vec3& nor, const Color& c)
      : coord(pos3, 1.0), normal(nor), color(c) {};
  };

  // Triangle template class, details depend on implementation of mesh
  template<typename VertexId>
  struct TriangleImp {
    VertexId a;  // Iterator to vertices
    VertexId b;
    VertexId c;

    TriangleImp(VertexId a, VertexId b, VertexId c) : a(a), b(b), c(c) {};
  };

  // Type alias
  using size_type = std::vector<Vertex>::size_type;
  using VertexIt = std::vector<Vertex>::iterator;
  using Triangle = TriangleImp<VertexIt>;
  using IndexTriangle = TriangleImp<size_type>;

  // Default Constructor : Dont allow empty mesh
  Mesh() = delete;

  // Copy controls
  ~Mesh() override = default;
  Mesh(const Mesh& rhs) = delete; // TODO do the extra work
  Mesh(Mesh&& rhs) = default;

  // Constructor with both vertices and triangles of vertex indices
  Mesh(const std::vector<Vertex>& va, const std::vector<size_type>& ta);
  Mesh(std::vector<Vertex>&& va, std::vector<size_type>&& ta);

  // Constructor alternatives
  Mesh(std::vector<Vertex>&& va, std::vector<IndexTriangle>&& ta);

  // Primitives queries
  const std::vector<Vertex>& get_vertices() const { return vertices; }
  const std::vector<Triangle>& get_triangles() const { return triangles; }
  const std::vector<IndexTriangle> get_indices() const;

private:
  std::vector<Vertex> vertices;
  std::vector<Triangle> triangles;

};


} // namespace CEL

#endif
