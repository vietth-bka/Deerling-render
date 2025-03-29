/**
 * @file camera.hpp
 * @brief Contains the Camera interface and related structures.
 */

#pragma once

#include <lightwave/color.hpp>
#include <lightwave/core.hpp>
#include <lightwave/math.hpp>
#include <lightwave/properties.hpp>
#include <lightwave/transform.hpp>

namespace lightwave {

/// @brief The result of sampling a Camera.
struct CameraSample {
    /// @brief The direction vector, pointing away from the camera.
    Ray ray;

    /// @brief The weight of the sample.
    Color weight;
};

/// @brief A Camera, representing the relationship between pixel coordinates and
/// rays.
class Camera : public Object {
protected:
    /// @brief The resolution of the image that is being rendered.
    Vector2i m_resolution;

    /// @brief The transform that leads from local coordinates to world space
    /// coordinates.
    ref<Transform> m_transform;

public:
    Camera(const Properties &properties) {
        m_resolution.x() = properties.get<int>("width");
        m_resolution.y() = properties.get<int>("height");
        m_transform      = properties.getChild<Transform>();

    }

    /// @brief Returns the resolution of the image that is being rendered.
    const Vector2i &resolution() const { return m_resolution; }

    /**
     * @brief Helper function to sample the camera model for a given pixel.
     * This function samples a random position within the given pixel,
     * normalizes the pixel coordinates, and then calls the @c
     * CameraSample::sample method for normalized pixel coordinates.
     *
     * @param pixel The pixel coordinates ranging from [0,0] to
     * [resolution().x() - 1, resolution.y() - 1].
     * @param rng A random number generator used to steer the sampling.
     */
    CameraSample sample(const Point2i &pixel, Sampler &rng) const;

    /**
     * @brief Samples a ray according to this camera model in world space
     * coordinates. Sampling begins in local coordinates following the
     * convention that [0,0,1] is the central viewing direction, and then
     * transforms the ray into world coordinates using the supplied @c
     * m_transform object.
     *
     * @param normalized Normalized coordinates ranging from [-1, -1] (bottom
     * left of the image) to [+1, +1] (top right of the image).
     * @param rng A random number generator used to steer the sampling.
     */
    virtual CameraSample sample(const Point2 &normalized,
                                Sampler &rng) const = 0;
};

} // namespace lightwave
