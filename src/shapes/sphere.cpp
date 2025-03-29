#include <lightwave.hpp>
#include <fstream>
#include <string>

// std::ofstream logFile("log.txt", std::ios::app);

namespace lightwave {
class Sphere : public Shape {
    const SphereSamplingMethod SphereSampling = SphereSamplingMethod::CosineWeighted;
protected:
    Point center;
    float radius;
    // float scaleFactor;

inline void populate(SurfaceEvent &surf, const Point &position) const {
        surf.position       = position;
        Vector normal       = (position - this->center).normalized();
        surf.shadingNormal  = normal;
        surf.geometryNormal = normal;
        surf.tangent        = Vector(0.f, normal[2], -normal[1]).normalized();
        surf.pdf = 0.25f * InvPi / sqr(this->radius);

        float u = 0.5f + atan2(normal.z(), normal.x()) / (2 * Pi); // Map longitude to [0, 1]
        float v = 0.5f + asin(normal.y()) / Pi;                 // Map latitude to [0, 1]
        // float u = 0.5f - atan2(normal.z(), normal.x()) / (2 * Pi); // Map longitude to [0, 1]
        // float v = 0.5f - asin(normal.y()) / Pi;                 // Map latitude to [0, 1]

        surf.uv = Point2(u, v);
        }

public:
Sphere(const Properties &properties) {
    this->center = Point(0.f, 0.f, 0.f);
    this->radius = 1.0f;
    // this->scaleFactor = properties.getChild<Transform>("transform", Transform())->scale().x();
}
bool intersect(const Ray &ray, Intersection &its, Sampler &rng) const override {
    PROFILE("Sphere")

    float t_1, t_2, t;
    const float A = ray.direction.dot(ray.direction);
    const float B = 2.0f * ray.direction.dot(ray.origin - this->center);
    const float C = (ray.origin - this->center).dot(ray.origin - this->center) - pow(this->radius,2);
    const float delta = pow(B, 2) - 4.f * A * C;

    if (delta < 0) {
        return false;
    }
    else {
        t_1 = (-B - sqrt(delta))/ (2.f*A);
        t_2 = (-B + sqrt(delta))/ (2.f*A);
    }
    
    // t = abs(t_1) < abs(t_2) ? t_1 : t_2;
    t = t_1 < Epsilon ? t_2 : t_1;

    if (t < Epsilon || t > its.t)
        return false;

    const Point position = ray(t);

    its.t = t;
    populate(its, position);
    // std::cout << "Intersection at position: " << position << std::endl;
    its.pdf = adjustpdf(ray.origin, position, SphereSampling);

    return true;

}

Bounds getBoundingBox() const override {
    return Bounds(Point(-1, -1, -1), Point(1, 1, 1));
}
Point getCentroid() const override {
    return this->center;
}
AreaSample sampleArea(Sampler &rng) const override {
    // NOT_IMPLEMENTED 
    Point position = squareToUniformSphere(rng.next2D());
    AreaSample sample;
    populate(sample, position); // compute the shading frame, texture coordinates
                        // and area pdf (same as intersection)
    return sample;
}

AreaSample improvedSampleArea(const Point &origin, Sampler &rng) const override {
    // NOT_IMPLEMENTED
    // remember to transform the origin to object space !!!
    Vector direct = origin - this->center;
    float distance = direct.length();
    if ((distance <= (this->radius + Epsilon)) || SphereSampling == SphereSamplingMethod::Uniform){
        return sampleArea(rng);
    }
    float cos_theta_max = this->radius / distance;
    float sin2theta_max = 1 - sqr(cos_theta_max);

    float u = rng.next2D().x(), v = rng.next2D().y();
    
    // std::string type = "cone";
    float x, y, z, phi;
    if (SphereSampling == SphereSamplingMethod::UniformCone){
        // uniform cone sampling
        z = 1 - u * (1 - cos_theta_max);
        phi = 2 * Pi * v;
        x = cos(phi) * sqrt(1 - sqr(z));
        y = sin(phi) * sqrt(1 - sqr(z));
    } else if (SphereSampling == SphereSamplingMethod::CosineWeighted) {
        // weighted cosine sampling
        z = sqrt(1 - u * sin2theta_max);
        phi = 2 * Pi * v;
        x = cos(phi) * sqrt(1 - sqr(z));
        y = sin(phi) * sqrt(1 - sqr(z));
    }
    
    Point sample_pos = Point(x, y, z);
    Point new_pos = ToOrientedCone(direct.normalized(), Vector(0.0f, 0.0f, 1.0f), sample_pos);

    AreaSample sample;
    populate(sample, new_pos);
    
    // if (SphereSampling == SphereSamplingMethod::UnifromCone){
    //     sample.pdf = Inv2Pi / (1 - cos_theta_max);      // for unifrom cone
    // } else if (SphereSampling == SphereSamplingMethod::CosineWeighted){
    //     sample.pdf = z * InvPi / sin2theta_max;    // for weighted cosine
    // } else {
    //     return sampleArea(rng);
    // }
    sample.pdf = adjustpdf(origin, new_pos, SphereSampling);

    return sample;
}

inline float adjustpdf(Point origin, Point hitpoint, const SphereSamplingMethod &method) const {
    Vector direct = origin - this->center;
    float distance = direct.length();
    float cos_theta_max = this->radius / distance;
    float sin2theta_max = 1 - sqr(cos_theta_max);
    if (method == SphereSamplingMethod::UniformCone){
        return Inv2Pi / (1 - cos_theta_max);
    } else if (method == SphereSamplingMethod::CosineWeighted){
        return (hitpoint-this->center).normalized().dot(direct.normalized()) * InvPi / sin2theta_max;
    } else {
        return 0.25f * InvPi;
    }
}

Point ToOrientedCone(const Vector &newaxis, const Vector &oldaxis, const Point &samplePoint) const{
    // project the sample point from canonical cone to a new oriented cone.
    // Rodigues's rotation method

    Vector r_axis = oldaxis.cross(newaxis).normalized();
    float cos_theta = oldaxis.dot(newaxis);
    float sin_theta = sqrt(1 - sqr(cos_theta));
    Matrix3x3 r_matrix;
    r_matrix.setRow(0, Vector(0.0f, -r_axis.z(), r_axis.y()));
    r_matrix.setRow(1, Vector(r_axis.z(), 0.0f, -r_axis.x()));
    r_matrix.setRow(2, Vector(-r_axis.y(), r_axis.x(), 0.0f));
    Matrix3x3 M = Matrix3x3::identity() + sin_theta * r_matrix + (1 - cos_theta) * r_matrix * r_matrix;

    Point newPoint = M * Vector(samplePoint);

    return newPoint;
}

std::string toString() const override {
return "Sphere[]";
}
};
}
REGISTER_SHAPE(Sphere, "sphere")


//--------------------------------------Save-here-as-a-backup----------------------------------------------------------------------------------------------------------------------------------------
// #include <lightwave.hpp>
// #include <fstream>
// #include <string>

// // std::ofstream logFile("log.txt", std::ios::app);

// namespace lightwave {
// class Sphere : public Shape {
// protected:
//     Point center;
//     float radius;
//     // float scaleFactor;

// inline void populate(SurfaceEvent &surf, const Point &position) const {
//         surf.position       = position;
//         Vector normal       = (position - this->center).normalized();
//         surf.shadingNormal  = normal;
//         surf.geometryNormal = normal;
//         surf.tangent        = Vector(0.f, normal[2], -normal[1]).normalized();
//         surf.pdf = 0.25f * InvPi / sqr(this->radius);

//         float u = 0.5f + atan2(normal.z(), normal.x()) / (2 * Pi); // Map longitude to [0, 1]
//         float v = 0.5f + asin(normal.y()) / Pi;                 // Map latitude to [0, 1]
//         // float u = 0.5f - atan2(normal.z(), normal.x()) / (2 * Pi); // Map longitude to [0, 1]
//         // float v = 0.5f - asin(normal.y()) / Pi;                 // Map latitude to [0, 1]

//         surf.uv = Point2(u, v);
//         }

// public:
// Sphere(const Properties &properties) {
//     this->center = Point(0.f, 0.f, 0.f);
//     this->radius = 1.0f;
//     // this->scaleFactor = properties.getChild<Transform>("transform", Transform())->scale().x();
// }
// bool intersect(const Ray &ray, Intersection &its, Sampler &rng) const override {
//     PROFILE("Sphere")

//     float t_1, t_2, t;
//     const float A = ray.direction.dot(ray.direction);
//     const float B = 2.0f * ray.direction.dot(ray.origin - this->center);
//     const float C = (ray.origin - this->center).dot(ray.origin - this->center) - pow(this->radius,2);
//     const float delta = pow(B, 2) - 4.f * A * C;

//     if (delta < 0) {
//         return false;
//     }
//     else {
//         t_1 = (-B - sqrt(delta))/ (2.f*A);
//         t_2 = (-B + sqrt(delta))/ (2.f*A);
//     }
    
//     // t = abs(t_1) < abs(t_2) ? t_1 : t_2;
//     t = t_1 < Epsilon ? t_2 : t_1;

//     if (t < Epsilon || t > its.t)
//         return false;

//     const Point position = ray(t);

//     its.t = t;
//     populate(its, position);
//     // std::cout << "Intersection at position: " << position << std::endl;

//     return true;

// }

// Bounds getBoundingBox() const override {
//     return Bounds(Point(-1, -1, -1), Point(1, 1, 1));
// }
// Point getCentroid() const override {
//     return this->center;
// }
// AreaSample sampleArea(Sampler &rng) const override {
//     // NOT_IMPLEMENTED 
//     Point position = squareToUniformSphere(rng.next2D());
//     AreaSample sample;
//     populate(sample, position); // compute the shading frame, texture coordinates
//                         // and area pdf (same as intersection)
//     return sample;
// }

// AreaSample improvedSampleArea(const Point &origin, Sampler &rng) const override {
//     // NOT_IMPLEMENTED
//     // remember to transform the origin to object space !!!
//     Vector direct = origin - this->center;
//     float distance = direct.length();
//     if (distance <= this->radius){
//         return sampleArea(rng);
//     }
//     float u = rng.next2D().x(), v = rng.next2D().y();

//     float cos_theta_max = this->radius / distance;
//     float sin2theta_max = 1 - sqr(cos_theta_max);
    
//     std::string type = "cone";
//     float x, y, z, phi;
//     if (type == "cone"){
//         // uniform cone sampling
//         z = 1 - u * (1 - cos_theta_max);
//         phi = 2 * Pi * v;
//         x = cos(phi) * sqrt(1 - sqr(z));
//         y = sin(phi) * sqrt(1 - sqr(z));
//     } else if (type == "cosine") {
//         // weighted cosine sampling
//         z = sqrt(1 - u * sin2theta_max);
//         phi = 2 * Pi * v;
//         x = cos(phi) * sqrt(1 - sqr(z));
//         y = sin(phi) * sqrt(1 - sqr(z));
//     } else {
//         return sampleArea(rng);
//     }
    
//     Point sample_pos = Point(x, y, z);
//     Point new_pos = ToOrientedCone(direct.normalized(), Vector(0.0f, 0.0f, 1.0f), sample_pos);

//     AreaSample sample;
//     populate(sample, new_pos);
    
//     if (type == "cone"){
//         sample.pdf = Inv2Pi / (1 - cos_theta_max);      // for unifrom cone
//     } else if (type == "cosine"){
//         sample.pdf = z * InvPi / sin2theta_max;    // for weighted cosine
//     } else {
//         return sampleArea(rng);
//     }

//     return sample;
// }

// Point ToOrientedCone(const Vector &newaxis, const Vector &oldaxis, const Point &samplePoint) const{
//     // project the sample point from canonical cone to a new oriented cone.
//     // Rodigues's rotation method

//     Vector r_axis = oldaxis.cross(newaxis).normalized();
//     float cos_theta = oldaxis.dot(newaxis);
//     float sin_theta = sqrt(1 - sqr(cos_theta));
//     Matrix3x3 r_matrix;
//     r_matrix.setRow(0, Vector(0.0f, -r_axis.z(), r_axis.y()));
//     r_matrix.setRow(1, Vector(r_axis.z(), 0.0f, -r_axis.x()));
//     r_matrix.setRow(2, Vector(-r_axis.y(), r_axis.x(), 0.0f));
//     Matrix3x3 M = Matrix3x3::identity() + sin_theta * r_matrix + (1 - cos_theta) * r_matrix * r_matrix;

//     Point newPoint = M * Vector(samplePoint);

//     return newPoint;
// }

// std::string toString() const override {
// return "Sphere[]";
// }
// };
// }
// REGISTER_SHAPE(Sphere, "sphere")