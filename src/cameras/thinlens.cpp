#include "lightwave/math.hpp"
#include <cmath>
#include <lightwave.hpp>

namespace lightwave {

class ThinLens final: public Camera {
public:
    ThinLens(const Properties &properties) : Camera(properties) {
        aspect_ratio_inv = (float)m_resolution.y() / m_resolution.x();
        m_fovAxis = properties.get<std::string>("fovAxis");
        m_fov = properties.get<float>("fov");
        if (m_fovAxis == "x")
            z = 1.f / std::tan(Deg2Rad * m_fov / 2);
        else
            z = aspect_ratio_inv / std::tan(Deg2Rad * m_fov / 2);
        lens_radius = properties.get<float>("lensRadius", 1);
        focal_distance = properties.get<float>("focalDistance", 1);
    }

    CameraSample sample(const Point2 &normalized, Sampler &rng) const override {
        auto ray_origin_screen =
            Ray(Vector(0.f, 0.f, 0.f),
                Vector(normalized.x(), normalized.y() * aspect_ratio_inv, z))
                .normalized();
        auto t = focal_distance / ray_origin_screen.direction.z();
        Point p_in_focus_plane = ray_origin_screen(t);
        Point2 sample = squareToUniformDiskConcentric(rng.next2D());
        Point lens_sample =
            Point(sample.x() * lens_radius, sample.y() * lens_radius, 0);
        auto direction = p_in_focus_plane - lens_sample;
        auto ray_from_focus_plane =
            Ray(p_in_focus_plane, direction).normalized();
        t = -focal_distance / ray_from_focus_plane.direction.z();
        Point ins_origin_plane = ray_from_focus_plane(t);
        auto ray_cam = Ray(ins_origin_plane, direction);
        auto ray_world = m_transform->apply(ray_cam).normalized();
        return CameraSample{.ray = ray_world, .weight = Color(1.0f)};
    }

    std::string toString() const override {
        return tfm::format(
            "ThinLens[\n"
            "  width = %d,\n"
            "  height = %d,\n"
            "  fovAxis = %s,n"
            "  transform = %s,\n"
            "  lensRadius = %4.2f,\n"
            "  focalDistance = %4.2f,\n"
            "]",
            m_resolution.x(),
            m_resolution.y(),
            m_fovAxis,
            indent(m_transform),
            lens_radius,
            focal_distance);
    }

private:
    std::string m_fovAxis;
    float m_fov, aspect_ratio_inv, z;
    float lens_radius, focal_distance;
};

}

REGISTER_CAMERA(ThinLens, "thinlens")
