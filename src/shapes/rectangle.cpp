#include "lightwave/properties.hpp"
#include "lightwave/registry.hpp"
#include "lightwave/sampler.hpp"
#include "lightwave/shape.hpp"

namespace lightwave {

struct SphQuad {
    /// @brief The implementation originates from the paper 
    /// https://blogs.autodesk.com/media-and-entertainment/wp-content/uploads/sites/162/egsr2013_spherical_rectangle.pdf
    Point origin;
    Vector v00;
    float x1, y1;
    float b0, b1;
    float k, S;

    SphQuad(const Point &origin) {
        this->origin = origin;
        v00 = Vector(-1 - origin.x(), -1 - origin.y(), -origin.z());
        // std::cout << "v00:" << v00 << "\n";
        x1 = v00.x() + 2;
        y1 = v00.y() + 2;
        Vector v01 = {v00.x(), y1,      v00.z()};
        Vector v10 = {x1,      v00.y(), v00.z()};
        Vector v11 = {x1,      y1,      v00.z()};

        Vector n0 = v00.cross(v10).normalized();
        Vector n1 = v01.cross(v00).normalized();
        Vector n2 = v11.cross(v01).normalized();
        Vector n3 = v10.cross(v11).normalized();

        float g0 = acos(-n0.dot(n1));
        float g1 = acos(-n1.dot(n2));
        float g2 = acos(-n2.dot(n3));
        float g3 = acos(-n3.dot(n0));

        b0 = n0.z();
        b1 = n2.z();
        k = 2 * Pi - g2 - g3;
        S = g0 + g1 - k;
    }

    Point sample(const Point2 &uv) {
        float au = uv.x() * S + k;
        float fu = -(cos(au) * b0 - b1) / sin(au); // used a negative sign here
        float cu = copysign(1.f, fu) / sqrt(sqr(fu) + sqr(b0));
        cu = clamp(cu, -1.f, 1.f);
        float xu = -(cu * v00.z()) / sqrt(1 - sqr(cu));
        // float xu = cu * v00.z() / sqrt(1 - sqr(cu));
        xu = clamp(xu, v00.x(), x1);
        float d2 = sqr(xu) + sqr(v00.z());
        float h0 = v00.y() / sqrt(d2 + sqr(v00.y()));
        float h1 = y1 / sqrt(d2 + sqr(y1));
        float hv = h0 + uv.y() * (h1 - h0);
        float hv2 = sqr(hv);
        float yv = (hv2 < 1 - Epsilon) ? (hv * sqrt(d2)) / sqrt(1 - hv2) : y1;

    return Point(xu + origin.x(), yv + origin.y(), 0);
    }
};



/// @brief A rectangle in the xy-plane, spanning from [-1,-1,0] to [+1,+1,0].
class Rectangle final : public Shape {

inline void populate(SurfaceEvent &surf, const Point &position) const {
        // surf.position = position;
        surf.position = {position.x(), position.y(), 0};
        // map the position from [-1,-1,0]..[+1,+1,0] to [0,0]..[1,1] by
        // discarding the z component and rescaling
        surf.uv.x() = (position.x() + 1) / 2;
        surf.uv.y() = (position.y() + 1) / 2;

        // the tangent always points in positive x direction
        surf.tangent = Vector(1, 0, 0);
        // and accordingly, the normal always points in the positive z direction
        surf.shadingNormal  = Vector(0, 0, 1);
        surf.geometryNormal = Vector(0, 0, 1);
        surf.pdf = 0.25;
    }


public:
    Rectangle(const Properties &properties) {}

    bool intersect(const Ray &ray, Intersection &its,
                    Sampler &rng) const override {

        // if the ray travels in the xy-plane, we report no intersection
        // (we ignore the edge case - pun intended - that the ray might have
        // infinite intersections with the rectangle)
        if (ray.direction.z() == 0)
            return false;

        // ray.origin.z + t * ray.direction.z = 0
        // <=> t = -ray.origin.z / ray.direction.z
        const float t = -ray.origin.z() / ray.direction.z();

        // note that we never report an intersection closer than Epsilon (to
        // avoid self-intersections)! we also do not update the intersection if
        // a closer intersection already exists (i.e., its.t is lower than our
        // own t)
        // if (t < Epsilon || t > its.t)
        //     return false;
        if (t < Epsilon || t > its.t)
            return false;

        // compute the hitpoint
        const Point position = ray(t);
        // we have intersected an infinite plane at z=0; now dismiss anything
        // outside of the [-1,-1,0]..[+1,+1,0] domain.
        if (std::abs(position.x()) > 1 || std::abs(position.y()) > 1)
            return false;

        // we have determined there was an intersection! we are now free to
        // change the intersection object and return true.
        its.t = t;
        populate(its, position); // compute the shading frame, texture coordinates
                            // and area pdf (same as sampleArea)
        auto squad = SphQuad(ray.origin);
        its.pdf = (ray.origin - position).normalized().z() / (squad.S * (position - ray.origin).lengthSquared());
        
        return true;
    }

    Bounds getBoundingBox() const override {
        return Bounds(Point{-1, -1, 0}, Point{+1, +1, 0});
    }

    Point getCentroid() const override { return Point(0); }

    AreaSample sampleArea(Sampler &rng) const override {
        Point2 rnd = rng.next2D(); // sample a random point in [0,0]..[1,1]
        Point position{
            2 * rnd.x() - 1, 2 * rnd.y() - 1, 0
        }; // stretch the random point to [-1,-1]..[+1,+1] and set z=0

        AreaSample sample;
        populate(sample,
                 position); // compute the shading frame, texture coordinates
                            // and area pdf (same as intersection)
        return sample;
    }

    AreaSample improvedSampleArea(const Point &origin, Sampler &rng) const override {
        if (abs(origin.z()) <= Epsilon) [[unlikely]]
            return AreaSample::invalid();
        auto squad = SphQuad(origin);
        if (squad.S < Epsilon) [[unlikely]]
            return AreaSample::invalid();
        Point position = squad.sample(rng.next2D());
        AreaSample sample;
        populate(sample, position);
        sample.pdf = abs((origin - position).normalized().z()) / (squad.S * (position - origin).lengthSquared());

        return sample;
    }

    std::string toString() const override { return "Rectangle[]"; }
};

}

// this informs lightwave to use our class Rectangle whenever a <shape type="rectangle" /> is found in a scene file
REGISTER_SHAPE(Rectangle, "rectangle")
