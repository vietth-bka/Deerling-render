#include <lightwave.hpp>

namespace lightwave {

class AreaLight final : public Light {
    Color light_power;
    Point light_source;
public:
    AreaLight(const Properties &properties) : Light(properties) {
        // std::cout << properties << "\n";
        light_power = properties.get<Color>("power");
        light_source = properties.get<Point>("position");
        // std::cout << "power " << power << "\n";
        // std::cout << "light_source " << light_source << "\n";
    }

    DirectLightSample sampleDirect(const Point &origin, Sampler &rng) const override {
        // float distance = (Vector(origin) - Vector(light_source)).length();
        float distance = (Vector(light_source) - Vector(origin)).length();
        Vector wi = (Vector(light_source) - Vector(origin)).normalized();
        Color weight = light_power / ((4 * Pi)*std::pow(distance, 2));
        // std::cout << "light_power " << light_power << "\n";
        // NOT_IMPLEMENTED
        return DirectLightSample{.wi=wi, .weight=weight, .distance=distance};
    }

    bool canBeIntersected() const override { return false; }

    std::string toString() const override {
        return tfm::format(
            "PointLight[\n"
            "]");
    }
};

} // namespace lightwave

REGISTER_LIGHT(AreaLight, "point")
