#include <lightwave.hpp>

namespace lightwave {

class PointLight final : public Light {
    Point m_position;
    Color m_power;
    Color intensity;
public:
    PointLight(const Properties &properties) : Light(properties) {
        m_position = properties.get<Point>("position");
        m_power = properties.get<Color>("power");
        intensity = m_power / (4 * Pi);
    }

    DirectLightSample sampleDirect(const Point &origin,
                                   Sampler &rng) const override {
        // NOT_IMPLEMENTED        
        /// @brief The direction vector, pointing from the query point towards the
        /// light.
        Vector wi = m_position - origin;
        /// @brief The distance from the query point to the sampled point on the
        /// light source.
        float distance = wi.length();
        
        /// @brief The weight of the sample, given by @code Le(-wi) / p(wi) @endcode
        Color weight = intensity / wi.lengthSquared();
        
        
        return DirectLightSample{
            .wi = wi.normalized(),
            .weight = weight,
            .distance = distance,
            .pdf = Infinity
        };

    }

    bool canBeIntersected() const override { return false; }

    std::string toString() const override {
        return tfm::format(
            "PointLight[\n"
            "]");
    }
};

} // namespace lightwave

REGISTER_LIGHT(PointLight, "point")
