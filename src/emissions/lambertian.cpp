#include <lightwave.hpp>

namespace lightwave {

class Lambertian : public Emission {
    ref<Texture> m_emission;

public:
    Lambertian(const Properties &properties) {
        m_emission = properties.get<Texture>("emission");
    }

    EmissionEval evaluate(const Point2 &uv, const Vector &wo) const override {
        // NOT_IMPLEMENTED
        if (wo.z() < 0)
            return EmissionEval::invalid();
            
        Color value = m_emission->evaluate(uv);
        // std::cout << "\bhit a lambertian surf" << std::endl;

        return EmissionEval{
            .value = value,
            .pdf = Inv2Pi
        };
    }

    std::string toString() const override {
        return tfm::format(
            "Lambertian[\n"
            "  emission = %s\n"
            "]",
            indent(m_emission));
    }
};

} // namespace lightwave

REGISTER_EMISSION(Lambertian, "lambertian")
