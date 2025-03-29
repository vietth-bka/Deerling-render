#include <lightwave.hpp>

#include "fresnel.hpp"
#include "microfacet.hpp"

namespace lightwave {

struct DiffuseLobe {
    Color color;

    BsdfEval evaluate(const Vector &wo, const Vector &wi) const {
        //NOT_IMPLEMENTED

        // hints:
        // * copy your diffuse bsdf evaluate here
        // * you do not need to query a texture, the albedo is given by `color`
        if (wo.z() * wi.z() < 0) {
            return BsdfEval::invalid();
        }
        float abs_costheta = Frame::absCosTheta(wi);

        // Return the Lambertian BSDF value: f(wi, wo) = albedo / pi
        return BsdfEval{
            .value = color * abs_costheta / Pi,
            .pdf   = abs(wi.z()) * InvPi
        };

    }

    BsdfSample sample(const Vector &wo, Sampler &rng) const {
        //NOT_IMPLEMENTED

        // hints:
        // * copy your diffuse bsdf evaluate here
        // * you do not need to query a texture, the albedo is given by `color`
        Vector wi = squareToCosineHemisphere(rng.next2D());
        if (wo.z() * wi.z() < 0) {
            wi.z() *= -1;
        }

        float pdf = Frame::sameHemisphere(wo, wi) ? Frame::absCosTheta(wi) * InvPi : 0;

        // weight = color/pi * costheta(wi) / pdf
        Color weight = color;

        return BsdfSample{
            .wi = wi,        
            .weight = weight,
            .pdf = pdf
        };
    }

    Color getAlbedo() const {
        return color;
    }
};

struct MetallicLobe {
    float alpha;
    Color color;

    BsdfEval evaluate(const Vector &wo, const Vector &wi) const {
        //NOT_IMPLEMENTED

        // hints:
        // * copy your roughconductor bsdf evaluate here
        // * you do not need to query textures
        //   * the reflectance is given by `color'
        //   * the variable `alpha' is already provided for you
        if (wo.z() * wi.z() <= 0){
            return BsdfEval::invalid();
        }

        Vector wh   = (wo + wi).normalized();
        Color R     = color;
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
    }

    BsdfSample sample(const Vector &wo, Sampler &rng) const {
        //NOT_IMPLEMENTED

        // hints:
        // * copy your roughconductor bsdf sample here
        // * you do not need to query textures
        //   * the reflectance is given by `color'
        //   * the variable `alpha' is already provided for you

        Vector wh   = microfacet::sampleGGXVNDF(alpha, wo, rng.next2D()).normalized();
        Vector wi   = reflect(wo, wh).normalized();

        if (wi.z() * wo.z() < 0){
            return BsdfSample::invalid();
        }

        float G1_w1 = microfacet::smithG1(alpha, wh, wi);
        Color R     = color;
        float D     = microfacet::evaluateGGX(alpha, wh);
        float G1_wo = microfacet::smithG1(alpha, wh, wo);

        Color weight = R * G1_w1; // cancelled out terms

        return{
            .wi     = wi,
            .weight = weight, // cos(theta) * B(wi, wo) / p(wi)
            .pdf   = abs(wo.z()) < Epsilon ? Infinity : D * G1_wo / (4 * abs(wo.z()))
        };
    }

    Color getAlbedo() const {
        return color;
    }
};

class Principled : public Bsdf {
    ref<Texture> m_baseColor;
    ref<Texture> m_roughness;
    ref<Texture> m_metallic;
    ref<Texture> m_specular;

    struct Combination {
        float diffuseSelectionProb;
        DiffuseLobe diffuse;
        MetallicLobe metallic;
    };

    Combination combine(const Point2 &uv, const Vector &wo) const {
        const auto baseColor = m_baseColor->evaluate(uv);
        const auto alpha = std::max(float(1e-3), sqr(m_roughness->scalar(uv)));
        const auto specular = m_specular->scalar(uv);
        const auto metallic = m_metallic->scalar(uv);
        const auto F =
            specular * schlick((1 - metallic) * 0.08f, Frame::cosTheta(wo));

        const DiffuseLobe diffuseLobe = {
            .color = (1 - F) * (1 - metallic) * baseColor,
        };
        const MetallicLobe metallicLobe = {
            .alpha = alpha,
            .color = F * Color(1) + (1 - F) * metallic * baseColor,
        };

        const auto diffuseAlbedo = diffuseLobe.color.mean();
        const auto totalAlbedo =
            diffuseLobe.color.mean() + metallicLobe.color.mean();
        return {
            .diffuseSelectionProb =
                totalAlbedo > 0 ? diffuseAlbedo / totalAlbedo : 1.0f,
            .diffuse  = diffuseLobe,
            .metallic = metallicLobe,
        };
    }

public:
    Principled(const Properties &properties) {
        m_baseColor = properties.get<Texture>("baseColor");
        m_roughness = properties.get<Texture>("roughness");
        m_metallic  = properties.get<Texture>("metallic");
        m_specular  = properties.get<Texture>("specular");
    }

    BsdfEval evaluate(const Point2 &uv, const Vector &wo,
                      const Vector &wi) const override {
        PROFILE("Principled")

        const auto combination = combine(uv, wo);
        // NOT_IMPLEMENTED

        // hint: evaluate `combination.diffuse` and `combination.metallic` and
        // combine their results

        BsdfEval diffuse_eval    = combination.diffuse.evaluate(wo, wi);
        BsdfEval metallic_eval   = combination.metallic.evaluate(wo, wi);

        return BsdfEval {
            .value = diffuse_eval.value + metallic_eval.value,
            .pdf = combination.diffuseSelectionProb * diffuse_eval.pdf
                + (1.0f - combination.diffuseSelectionProb) * metallic_eval.pdf
        };

    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {
        PROFILE("Principled")

        const auto combination = combine(uv, wo);
        // NOT_IMPLEMENTED

        // hint: sample either `combination.diffuse` (probability
        // `combination.diffuseSelectionProb`) or `combination.metallic`
        float p_sample = rng.next();
        float P = combination.diffuseSelectionProb;
        if (p_sample < P){
            BsdfSample sample_lobe = combination.diffuse.sample(wo, rng);
            return BsdfSample {
                .wi     = sample_lobe.wi,
                .weight = sample_lobe.weight / P,
                .pdf = P * sample_lobe.pdf
            };
        }
        
            BsdfSample sample_lobe = combination.metallic.sample(wo, rng);
            return BsdfSample{
                .wi     = sample_lobe.wi,
                .weight = sample_lobe.weight / (1.f - P),
                .pdf = (1.0f - P) * sample_lobe.pdf
        };
    }

    Color getAlbedo(const Intersection &its) const override {
            const auto combination = combine(its.uv, its.wo);
            Color diffuse_sample = combination.diffuse.getAlbedo();
            Color metallic_sample = combination.metallic.getAlbedo();
            float prob = combination.diffuseSelectionProb;

            return diffuse_sample*prob + metallic_sample*(1-prob);

        }

    std::string toString() const override {
        return tfm::format("Principled[\n"
                           "  baseColor = %s,\n"
                           "  roughness = %s,\n"
                           "  metallic  = %s,\n"
                           "  specular  = %s,\n"
                           "]",
                           indent(m_baseColor), indent(m_roughness),
                           indent(m_metallic), indent(m_specular));
    }
};

} // namespace lightwave

REGISTER_BSDF(Principled, "principled")
