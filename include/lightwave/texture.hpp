/**
 * @file texture.hpp
 * @brief Contains the Texture interface, which models spatially varying
 * properties of materials.
 */

#pragma once

#include <lightwave/color.hpp>
#include <lightwave/core.hpp>
#include <lightwave/math.hpp>

namespace lightwave {

/// @brief Models spatially varying material properties (e.g., images or
/// procedural noise).
class Texture : public Object {
public:
    /**
     * @brief Returns the color at a given texture coordinate.
     * For most applications, the input point will lie in the unit square
     * [0,1)^2, but points outside this domain are also allowed.
     */
    virtual Color evaluate(const Point2 &uv) const = 0;
    /**
     * @brief Returns a scalar value at a given texture coordinate.
     * For most applications, the input point will lie in the unit square
     * [0,1)^2, but points outside this domain are also allowed.
     */
    virtual float scalar(const Point2 &uv) const {
        // arbitrary mapping from RGB images to scalar values (typically those
        // will be grayscale anyway and we would ideally have a separate texture
        // interface for scalar values)
        return evaluate(uv).r();
    }
};

class ImageTexture : public Texture {
public:
    enum class BorderMode {
        Clamp,
        Repeat,
    };

    enum class FilterMode {
        Nearest,
        Bilinear,
    };

    ref<Image> m_image;
    float m_exposure;
    BorderMode m_border;
    FilterMode m_filter;
    int width, height;
public:
    ImageTexture(const Properties &properties);

    Color filterNearest(const Point2 &uv) const; 
    

    Color filterBilinear(const Point2 &uv) const;

    Color evaluate(const Point2 &uv) const override;

    std::string toString() const override {
    return tfm::format(
        "ImageTexture[\n"
        "  image = %s,\n"
        "  exposure = %f,\n"
        "]",
        indent(m_image),
        m_exposure);
    }

    };

} // namespace lightwave
