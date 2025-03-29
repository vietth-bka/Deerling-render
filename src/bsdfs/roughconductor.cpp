#include "fresnel.hpp"
#include "microfacet.hpp"
#include <lightwave.hpp>

namespace lightwave {

class RoughConductor : public Bsdf {
    ref<Texture> m_reflectance;
    ref<Texture> m_roughness;

public:
    RoughConductor(const Properties &properties) {
        m_reflectance = properties.get<Texture>("reflectance");
        m_roughness   = properties.get<Texture>("roughness");
    }

    BsdfEval evaluate(const Point2 &uv, const Vector &wo,
                      const Vector &wi) const override {
        // Using the squared roughness parameter results in a more gradual
        // transition from specular to rough. For numerical stability, we avoid
        // extremely specular distributions (alpha values below 10^-3)
        const auto alpha = std::max(float(1e-3), sqr(m_roughness->scalar(uv)));
        
        if (!Frame::sameHemisphere(wo, wi)){
            return BsdfEval::invalid();
        }
        // should add costheta check here because wi may go to the opposite side 

        // NOT_IMPLEMENTED
        Vector wh   = (wo + wi).normalized();
        Color R     = m_reflectance->evaluate(uv);
        float D     = microfacet::evaluateGGX(alpha, wh);
        float G1_wi = microfacet::smithG1(alpha, wh, wi);
        float G1_wo = microfacet::smithG1(alpha, wh, wo);
        float coso  = Frame::absCosTheta(wo);

        if (coso == 0){
            return BsdfEval::invalid();
        }

        return {
            .value = Color(R * D * G1_wi * G1_wo / (4 * coso)), // cos(theta) * B(wi, wo)
            .pdf   = abs(wo.z()) < Epsilon ? Infinity : D * G1_wo / (4 * abs(wo.z()))
        };
        // hints:
        // * the microfacet normal can be computed from `wi' and `wo'
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {
        const auto alpha = std::max(float(1e-3), sqr(m_roughness->scalar(uv)));

        // NOT_IMPLEMENTED

        Vector wh   = microfacet::sampleGGXVNDF(alpha, wo, rng.next2D()).normalized();
        Vector wi   = reflect(wo, wh).normalized();
        // should add costheta check here because wi may go to the opposite side 
        if (!Frame::sameHemisphere(wo, wi)){
            return BsdfSample::invalid();
        }

        float G1_w1 = microfacet::smithG1(alpha, wh, wi);
        float G1_wo = microfacet::smithG1(alpha, wh, wo);
        Color R     = m_reflectance->evaluate(uv);
        float D     = microfacet::evaluateGGX(alpha, wh);

        Color weight = R * G1_w1; // cos(theta) * B(wi, wo) / p(wi)

        return{
            .wi     = wi,
            .weight = weight, 
            .pdf    = abs(wo.z()) < Epsilon ? Infinity : D * G1_wo / (4 * abs(wo.z()))
        };
        // hints:
        // * do not forget to cancel out as many terms from your equations as
        // possible!
        //   (the resulting sample weight is only a product of two factors)
    }

    Color getAlbedo(const Intersection &its) const override {
        return 0.5f * (m_reflectance->evaluate(its.uv) + m_roughness->evaluate(its.uv));
    }

    std::string toString() const override {
        return tfm::format(
            "RoughConductor[\n"
            "  reflectance = %s,\n"
            "  roughness = %s\n"
            "]",
            indent(m_reflectance),
            indent(m_roughness));
    }
};

} // namespace lightwave

REGISTER_BSDF(RoughConductor, "roughconductor")