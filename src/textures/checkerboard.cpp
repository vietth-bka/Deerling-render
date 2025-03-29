#include <lightwave.hpp>

namespace lightwave {

class CheckerboardTexture : public Texture {    
    Color color0, color1;
    Vector2 m_scale;

public:
    CheckerboardTexture(const Properties &properties) {
        color0 = properties.get<Color>("color0", Color(0.0f));
        color1 = properties.get<Color>("color1", Color(0.0f));
        m_scale  = properties.get<Vector2>("scale");

    }

    Color evaluate(const Point2 &uv) const override {
        // Scale the UV coordinates
        float u = uv.x() * m_scale.x();
        float v = uv.y() * m_scale.y();

        // Compute checkerboard pattern
        int checker = (int(std::floor(u)) + int(std::floor(v))) % 2;

        // Alternate between color0 and color1
        return (checker == 0) ? color0 : color1;

    }

    std::string toString() const override {
        return tfm::format(
            "CheckerboardTexture[\n"
            "]");
    }
};

} // namespace lightwave

REGISTER_TEXTURE(CheckerboardTexture, "checkerboard")
