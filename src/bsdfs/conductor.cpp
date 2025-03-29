#include <lightwave.hpp>

namespace lightwave {

class Conductor : public Bsdf {
    ref<Texture> m_reflectance;

public:
    Conductor(const Properties &properties) {
        m_reflectance = properties.get<Texture>("reflectance");
    }

    BsdfEval evaluate(const Point2 &uv, const Vector &wo,
                      const Vector &wi) const override {
        // the probability of a light sample picking exactly the direction `wi'
        // that results from reflecting `wo' is zero, hence we can just ignore
        // that case and always return black
        return BsdfEval::invalid();
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {
        // NOT_IMPLEMENTED
        Vector n(0.0f, 0.0f, 1.0f);
        Vector wi = reflect(wo, n).normalized();

        if (!Frame::sameHemisphere(wo, wi)) {
            return BsdfSample::invalid();
        }
        // if (wi.z() <= 0.0f) {
        //     return BsdfSample::invalid();
        // } should not make conductor one-sided !

        Color weight = m_reflectance->evaluate(uv);

        return BsdfSample{
            .wi = wi,
            .weight = weight,
            .pdf = Infinity
        };

    }

    Color getAlbedo(const Intersection &its) const override {
            return m_reflectance->evaluate(its.uv); // Compute the BSDF reflectance value as albedo
        }

    std::string toString() const override {
        return tfm::format("Conductor[\n"
                           "  reflectance = %s\n"
                           "]",
                           indent(m_reflectance));
    }
};

} // namespace lightwave

REGISTER_BSDF(Conductor, "conductor")
