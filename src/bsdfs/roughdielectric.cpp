#include "fresnel.hpp"
#include "microfacet.hpp"
#include <lightwave.hpp>

namespace lightwave {

class RoughDielectric : public Bsdf {
    ref<Texture> m_ior;
    ref<Texture> m_reflectance;
    ref<Texture> m_transmittance;
    ref<Texture> m_roughness;

public:
    RoughDielectric(const Properties &properties) {
        m_ior           = properties.get<Texture>("ior");
        m_reflectance   = properties.get<Texture>("reflectance");
        m_transmittance = properties.get<Texture>("transmittance");
        m_roughness     = properties.get<Texture>("roughness");
    }

    BsdfEval evaluate(const Point2 &uv, const Vector &wo,
                      const Vector &wi) const override {        

        // return BsdfEval::invalid();
        auto alpha = std::max(float(1e-3), sqr(m_roughness->scalar(uv)));
        // Path regularization makes sure perfect specular surfaces will become
        // non-perfect specular
        // if (alpha < 0.3f) 
        //     alpha = clamp(2 * alpha, 0.1f, 0.3f);

        float cosTheta_o = Frame::cosTheta(wo), cosTheta_i = Frame::cosTheta(wi);
        bool reflect = cosTheta_o * cosTheta_i > 0;
        float etap = 1;
        float eta = m_ior->scalar(uv);
        if (!reflect){
            etap = cosTheta_o > 0 ? eta : 1 / eta;
        }
        Vector wh = wi * etap + wo;
        if (cosTheta_i == 0 || cosTheta_o == 0 || wh.lengthSquared() == 0){
            return BsdfEval::invalid();
        }
        wh = wh.normalized();
        wh = wh.z() > 0 ? wh : -wh;
        
        if (wi.dot(wh) * cosTheta_i < 0 || wo.dot(wh) * cosTheta_o < 0){
            return BsdfEval::invalid();
        }

        float D     = microfacet::evaluateGGX(alpha, wh);
        float G1_wi = microfacet::smithG1(alpha, wh, wi);
        float G1_wo = microfacet::smithG1(alpha, wh, wo);
        Color R     = m_reflectance->evaluate(uv);
        Color T     = m_transmittance->evaluate(uv);

        if (reflect){
            float F = fresnelDielectric(wo.dot(wh), cosTheta_o > 0 ? eta: 1/eta);
            return BsdfEval{
                .value = Color(F * R * D * G1_wi * G1_wo / (4 * abs(cosTheta_o))),
                // .pdf = D * G1_wo / (4 * abs(wo.z()))
                .pdf   = F * microfacet::pdfGGXVNDF(alpha, wh, wo) * microfacet::detReflection(wh, wo)
            };
        } else {
            // float F = fresnelDielectric(wi.dot(wh), etap);
            float F = fresnelDielectric(wo.dot(wh), etap);

            return BsdfEval{
                .value = Color((1-F) * T * D * G1_wi * G1_wo * abs(wi.dot(wh)) * abs(wo.dot(wh)) / 
                        (sqr(wi.dot(wh) + wo.dot(wh)/etap) * abs(cosTheta_o))),
                // .pdf   = D * G1_wo * abs(wo.dot(wh)) * (1-F) / abs(wo.z()) * microfacet::detRefraction(wh, wi, wo, etap)
                .pdf   = (1-F) * microfacet::pdfGGXVNDF(alpha, wh, wo) * microfacet::detRefraction(wh, wi, wo, etap)
            };
        }
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo, 
                        Sampler &rng) const override {
        // NOT_IMPLEMENTED
        const float ior = m_ior->scalar(uv);
        auto alpha = std::max(float(1e-3), sqr(m_roughness->scalar(uv)));
        // Path regularization - makes sure perfect specular surfaces will become
        // non-perfect specular
        // if (alpha < 0.3f) 
        //     alpha = clamp(2 * alpha, 0.1f, 0.3f);

        Vector wh   = microfacet::sampleGGXVNDF(alpha, wo, rng.next2D()).normalized();
        float eta = wo.z() > 0 ? ior : 1/ior;

        float F = fresnelDielectric(wo.dot(wh), eta);
        float p = rng.next();

        Vector wi = Vector(0.0f, 0.0f, 0.0f);
        Color weight = Color(0.0f);
        float pdf = 1.0f;

        if (p < F) //Reflect
        {
            wi = reflect(wo, wh).normalized();
            if (!Frame::sameHemisphere(wo, wi)){
                return BsdfSample::invalid();
            }
            float G1_wi = microfacet::smithG1(alpha, wh, wi);            
            Color R     = m_reflectance->evaluate(uv);
            // cos(theta) * B(wi, wo) / p(wi)
            weight = R * G1_wi; // reduced form
            pdf = F * microfacet::pdfGGXVNDF(alpha, wh, wo) * microfacet::detReflection(wh, wo);
        }
        else{       //Refract
            wi = refract(wo, wh, eta);
            if (Frame::sameHemisphere(wo, wi)){
                return BsdfSample::invalid();
            }
            Color T = m_transmittance->evaluate(uv);
            float G1_wi = microfacet::smithG1(alpha, wh, wi);            
            pdf = (1-F) * microfacet::pdfGGXVNDF(alpha, wh, wo) * microfacet::detRefraction(wh, wi, wo, eta);
            // Color f_t   = (1-F) * T * D * G1_wi * G1_wo * abs(wi.dot(wh)) * abs(wo.dot(wh)) /
            //         (sqr(wi.dot(wh) + wo.dot(wh)/eta) * abs(wo.z()) * abs (wi.z()));
            // weight = f_t * Frame::absCosTheta(wi) / pdf;
            weight = T * G1_wi / sqr(eta); // transportmode: radiance, reduced form
            
            if (wi.isZero()){ // check if the total internal reflection happens or not
                // return BsdfSample::invalid();
                return {.wi = reflect(wo, wh).normalized(), 
                        .weight = m_reflectance->evaluate(uv) * G1_wi, 
                        // .pdf = D * G1_wo * F / (4 * abs(wo.z())),
                        .pdf = F * microfacet::pdfGGXVNDF(alpha, wh, wo) * microfacet::detReflection(wh, wo),
                        .eta = eta
                        };
            }
        }
        return BsdfSample{
            .wi = wi,
            .weight = weight,
            .pdf = pdf,
            .eta = eta
        };
    }

    Color getAlbedo(const Intersection &its) const override{
            float ior = m_ior->scalar(its.uv);
            float eta = Frame::cosTheta(its.wo) > 0 ? ior : 1/ior;
            float F = fresnelDielectric(Frame::cosTheta(its.wo), eta); 
            Color reflectance = m_reflectance->evaluate(its.uv);
            Color transmittance = m_transmittance->evaluate(its.uv);
            return reflectance * F + transmittance * (1.0f - F) / sqr(eta);
        }

    std::string toString() const override {
        return tfm::format(
            "RoughDielectric[\n"
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

REGISTER_BSDF(RoughDielectric, "roughdielectric");
