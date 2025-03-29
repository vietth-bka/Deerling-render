#include <lightwave.hpp>
#include <iostream> 

namespace lightwave {

/**
 * @brief A perspective camera with a given field of view angle and transform.
 *
 * In local coordinates (before applying m_transform), the camera looks in
 * positive z direction [0,0,1]. Pixels on the left side of the image ( @code
 * normalized.x < 0 @endcode ) are directed in negative x direction ( @code
 * ray.direction.x < 0 ), and pixels at the bottom of the image ( @code
 * normalized.y < 0 @endcode ) are directed in negative y direction ( @code
 * ray.direction.y < 0 ).
 */
class Perspective : public Camera {
protected:
    float fovRad;
    std::string fovAxis;
    float scaleX, scaleY;
public:
    Perspective(const Properties &properties) : Camera(properties) {
        // NOT_IMPLEMENTED
        float fov = properties.get<float>("fov");
        this->fovAxis = properties.get<std::string>("fovAxis");
        this->fovRad = fov / 180.0f * Pi;
        float aspectRatio = float(m_resolution.x()) / float(m_resolution.y());

        if (this->fovAxis == "x"){
            scaleX = 1;
            scaleY = 1 / aspectRatio;
        }
        else if (this->fovAxis == "y") {
            scaleX = aspectRatio;
            scaleY = 1;
        }
        // hints:
        // * precompute any expensive operations here (most importantly
        // trigonometric functions)
        // * use m_resolution to find the aspect ratio of the image
    }

    CameraSample sample(const Point2 &normalized, Sampler &rng) const override { 
        // Normalized point coordinates in range [-1, 1] in screen space
        // Normalized case
        float newX = normalized.x() * scaleX;
        float newY = normalized.y() * scaleY;
        float focallen = 1 / tanf(this->fovRad / 2.0f);
        
        Point origin = Point(0.f, 0.f, 0.f);
        Vector d = Vector(newX, newY, focallen);
        Ray localray = Ray(origin, d.normalized());
        Ray worldray = this->m_transform->apply(localray);


        return CameraSample{.ray = worldray.normalized(),
                            .weight = Color(1.0f)};


        // hints:
        // * use m_transform to transform the local camera coordinate system
        // into the world coordinate system
    }

    std::string toString() const override {
        return tfm::format("Perspective[\n"
                           "  width = %d,\n"
                           "  height = %d,\n"
                           "  transform = %s,\n"
                           "]",
                           m_resolution.x(), m_resolution.y(),
                           indent(m_transform));
    }
};

} // namespace lightwave

REGISTER_CAMERA(Perspective, "perspective")
