#include <lightwave.hpp>

#include "../core/plyparser.hpp"
#include "accel.hpp"

// #include <fstream>

// std::ofstream logFile("log.txt", std::ios::app);

namespace lightwave {

/**
 * @brief A shape consisting of many (potentially millions) of triangles, which
 * share an index and vertex buffer. Since individual triangles are rarely
 * needed (and would pose an excessive amount of overhead), collections of
 * triangles are combined in a single shape.
 */
class TriangleMesh : public AccelerationStructure {
    /**
     * @brief The index buffer of the triangles.
     * The n-th element corresponds to the n-th triangle, and each component of
     * the element corresponds to one vertex index (into @c m_vertices ) of the
     * triangle. This list will always contain as many elements as there are
     * triangles.
     */
    std::vector<Vector3i> m_triangles; // A four-dimensional vector with floating point components (used for homogeneous coordinates).
    /**
     * @brief The vertex buffer of the triangles, indexed by m_triangles.
     * Note that multiple triangles can share vertices, hence there can also be
     * fewer than @code 3 * numTriangles @endcode vertices.
     */
    std::vector<Vertex> m_vertices;
    /// @brief The file this mesh was loaded from, for logging and debugging
    /// purposes.
    std::filesystem::path m_originalPath;
    /// @brief Whether to interpolate the normals from m_vertices, or report the
    /// geometric normal instead.
    bool m_smoothNormals;

    inline void populate(SurfaceEvent &surf, const Point &position, Vector shadingNormal, Vector normal, Vector2 uv_map) const {
        surf.position = position;
        // surf.tangent = Vector(0.f, normal[2], -normal[1]).normalized();
        surf.tangent = Vector(0.f, shadingNormal[2], -shadingNormal[1]);
        if (surf.tangent.isZero()){
            surf.tangent = Vector(0.0f, 0.0f, 1.0f);
        } else {
            surf.tangent = surf.tangent.normalized();
        }
        surf.shadingNormal  = shadingNormal;
        surf.geometryNormal = normal;

        surf.uv.x() = uv_map.x();
        // surf.uv.y() = 1 - uv_map.y();
        surf.uv.y() = uv_map.y();

        surf.pdf = 0.0f / 4;
        }

protected:
    int numberOfPrimitives() const override { return int(m_triangles.size()); }

    bool intersect(int primitiveIndex, const Ray &ray, Intersection &its,
                   Sampler &rng) const override {
        // NOT_IMPLEMENTED

        // hints:
        // * use m_triangles[primitiveIndex] to get the vertex indices of the
        // triangle that should be intersected
        // * if m_smoothNormals is true, interpolate the vertex normals from
        // m_vertices
        //   * make sure that your shading frame stays orthonormal!
        // * if m_smoothNormals is false, use the geometrical normal (can be
        // computed from the vertex positions)
        Vector3i v_indices = m_triangles[primitiveIndex];
        Vertex A = m_vertices[v_indices[0]];
        Vertex B = m_vertices[v_indices[1]];
        Vertex C = m_vertices[v_indices[2]];
        Point a = A.position;
        Point b = B.position;
        Point c = C.position;

        Point ray_ori = ray.origin;
        Vector d = ray.direction;

        //Möller-Trumbore algorithm
        // o + td = (1-u-v)a + ub + vc <=> o-a = -td + u(b-a) + v(c-a)
        Vector ba = b - a;
        Vector ca = c - a;
        Vector s = ray_ori - a;
        Vector geoNormal = ba.cross(ca).normalized();
        Vector shadingNormal = geoNormal;
        // s = -t*d + u*ba + v*ca

        float det = d.dot(ba.cross(ca));
        float check = d.dot(geoNormal);

        if (-Epsilon < check && check < Epsilon) {
            return false;
        }

        float inv_det = 1.f / det;
        float u = d.dot(s.cross(ca)) * inv_det;
        // if ((u < 0 && abs(u) > Epsilon) || (u > 1 && abs(u-1) > Epsilon))
        //     return false;
        if (u < 0 || u > 1) 
            return false;

        float v = d.dot((ba).cross(s)) * inv_det;
        // if ((v < 0 && abs(v) > Epsilon) || (u + v > 1 && abs(u + v - 1) > Epsilon))
        //     return false;
        if (v < 0 || (u + v > 1)) 
            return false;

        float t = -s.dot(ba.cross(ca)) * inv_det;        
        // Done with Möller-Trumbore

        if (t < Epsilon || t > its.t)
            return false;
            
        if (m_smoothNormals) {
            shadingNormal = Vertex::interpolate(Vector2(u, v), A, B, C).normal.normalized();
        }

        const Vector2 uv_map = Vertex::interpolate(Vector2(u, v), A, B, C).uv;

        const Point position = ray(t);
        its.t = t;
        // std::cout << "geoNormal: " << geoNormal << std::endl;
        populate(its, position, shadingNormal, geoNormal, uv_map);

        return true;
    }

    Bounds getBoundingBox(int primitiveIndex) const override {
        Vector3i v_indices = m_triangles[primitiveIndex];
        Point a = m_vertices[v_indices[0]].position;
        Point b = m_vertices[v_indices[1]].position;
        Point c = m_vertices[v_indices[2]].position;
        float min_x = min(min(a[0], b[0]), c[0]);
        float min_y = min(min(a[1], b[1]), c[1]);
        float min_z = min(min(a[2], b[2]), c[2]);
        float max_x = max(max(a[0], b[0]), c[0]);
        float max_y = max(max(a[1], b[1]), c[1]);
        float max_z = max(max(a[2], b[2]), c[2]);
        return Bounds(Point(min_x, min_y, min_z), Point(max_x, max_y, max_z));
    }

    Point getCentroid(int primitiveIndex) const override {
        Vector3i v_indices = m_triangles[primitiveIndex];
        Vector a = Vector(m_vertices[v_indices[0]].position);
        Vector b = Vector(m_vertices[v_indices[1]].position);
        Vector c = Vector(m_vertices[v_indices[2]].position);
        Point centroid = Point((a + b + c)/3.f);
        return centroid;
    }

public:
    TriangleMesh(const Properties &properties) {
        m_originalPath  = properties.get<std::filesystem::path>("filename");
        m_smoothNormals = properties.get<bool>("smooth", true);
        readPLY(m_originalPath, m_triangles, m_vertices);
        logger(EInfo,
               "loaded ply with %d triangles, %d vertices",
               m_triangles.size(),
               m_vertices.size());
        buildAccelerationStructure();
    }

    bool intersect(const Ray &ray, Intersection &its,
                   Sampler &rng) const override {
        PROFILE("Triangle mesh")
        return AccelerationStructure::intersect(ray, its, rng);
    }

    AreaSample sampleArea(Sampler &rng) const override{
        // only implement this if you need triangle mesh area light sampling for
        // your rendering competition
        NOT_IMPLEMENTED
    }

    std::string toString() const override {
        return tfm::format(
            "Mesh[\n"
            "  vertices = %d,\n"
            "  triangles = %d,\n"
            "  filename = \"%s\"\n"
            "]",
            m_vertices.size(),
            m_triangles.size(),
            m_originalPath.generic_string());
    }
};

} // namespace lightwave

REGISTER_SHAPE(TriangleMesh, "mesh")
