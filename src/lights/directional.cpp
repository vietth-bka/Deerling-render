#include <lightwave.hpp>

namespace lightwave {

class DirectionalLight final : public Light {
public:
    Vector direction;
    Color intensity;

    DirectionalLight(const Properties &properties) : Light(properties) {
        direction = properties.get<Vector>("direction").normalized();
        intensity = properties.get<Color>("intensity");
    }

    DirectLightSample sampleDirect(const Point &origin, Sampler &rng) const override {
        // Directional lights do not have a position, so distance is effectively infinite.
        // We assume the rays are parallel and all come from the same direction.
        return DirectLightSample{
            .wi = direction,
            .weight = intensity,
            .distance = Infinity,
            .pdf = Infinity
        };
    }

    std::string toString() const override {
        return tfm::format(
            "DirectionalLight[\n",
            "]");
    }
};

} // namespace lightwave

REGISTER_LIGHT(DirectionalLight, "directional")
