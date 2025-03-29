#include "fresnel.hpp"
#include <lightwave.hpp>

namespace lightwave {

class Dielectric : public Bsdf {
    ref<Texture> m_ior;
    ref<Texture> m_reflectance;
    ref<Texture> m_transmittance;

public:
    Dielectric(const Properties &properties) {
        m_ior           = properties.get<Texture>("ior");
        m_reflectance   = properties.get<Texture>("reflectance");
        m_transmittance = properties.get<Texture>("transmittance");
    }

    BsdfEval evaluate(const Point2 &uv, const Vector &wo,
                      const Vector &wi) const override {
        // the probability of a light sample picking exactly the direction `wi'
        // that results from reflecting or refracting `wo' is zero, hence we can
        // just ignore that case and always return black
        return BsdfEval::invalid();
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {
        // NOT_IMPLEMENTED
        float ior = m_ior->scalar(uv);
        Color reflectance = m_reflectance->evaluate(uv);
        Color transmittance = m_transmittance->evaluate(uv);

        bool sign = Frame::cosTheta(wo) > 0;
        float eta = sign ? ior : 1.0f/ior;

        float cosThetaI = Frame::absCosTheta(wo);
        float F = fresnelDielectric(cosThetaI, eta);

        Vector wi = Vector(0.0f, 0.0f, 0.0f);
        Color weight = Color(0.0f);
        float pdf = F;

        if (rng.next() < F) //Reflect
        {
            wi = reflect(wo, Vector(0.0f, 0.0f, 1.0f));
            weight = reflectance;
            pdf = F;
        }
        else{       //Refract
            wi = refract(wo, Vector(0.0f, 0.0f, 1.0f), eta);            
            weight = transmittance / sqr(eta);
            pdf = 1.0f - F;
        }
        // check if the total internal reflection happens or not
        if (wi.isZero()){
            return {.wi = reflect(wo, Vector(0.0f, 0.0f, 1.0f)).normalized(), 
                    .weight = reflectance, 
                    .pdf = F * (1.0f - F), 
                    .eta = eta};
        }
        return BsdfSample{
            .wi = wi,
            .weight = weight,
            .pdf = Infinity,
            .eta = eta
        };
    }

    Color getAlbedo(const Intersection &its) const override
        {
            float ior = m_ior->scalar(its.uv);
            float eta = Frame::cosTheta(its.wo) > 0 ? ior : 1/ior;
            float F = fresnelDielectric(Frame::cosTheta(its.wo), eta); 
            Color reflectance = m_reflectance->evaluate(its.uv);
            Color transmittance = m_transmittance->evaluate(its.uv);
            return reflectance * F + transmittance * (1.0f - F) / sqr(eta);
        }

    std::string toString() const override {
        return tfm::format(
            "Dielectric[\n"
            "  ior           = %s,\n"
            "  reflectance   = %s,\n"
            "  transmittance = %s\n"
            "]",
            indent(m_ior),
            indent(m_reflectance),
            indent(m_transmittance));
    }
};

} // namespace lightwave

REGISTER_BSDF(Dielectric, "dielectric")
