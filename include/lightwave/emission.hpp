/**
 * @file emission.hpp
 * @brief Contains the Emission interface and related structures.
 */

#pragma once

#include <lightwave/color.hpp>
#include <lightwave/core.hpp>
#include <lightwave/math.hpp>

namespace lightwave {

/// @brief The result of evaluating an @ref Emission .
struct EmissionEval {
    /// @brief The color of the emission, not including any cosine term.
    Color value;

    float pdf;

    static EmissionEval invalid() {
        return {
            .value = Color(0),
            .pdf   = 0.0f,
        };
    }

    /// @brief Reports whether an object has been hit.
    explicit operator bool() const { return value != Color::black(); }
};

/// @brief An Emission, representing the light distribution of emissive
/// surfaces.
class Emission : public Object {
public:
    Emission() {}

    /**
     * @brief Evaluates the emission for a given direction in local coordinates
     * (i.e., the normal is assumed to be [0,0,1]).
     *
     * @param uv The texture coordinates of the surface.
     * @param wo The outgoing direction light is emitted in, pointing away from
     * the surface, in local coordinates.
     */
    virtual EmissionEval evaluate(const Point2 &uv, const Vector &wo) const = 0;
};

} // namespace lightwave
