#include <lightwave.hpp>

namespace lightwave {

class Diffuse : public Bsdf {
    ref<Texture> m_albedo;

public:
    Diffuse(const Properties &properties) {
        m_albedo = properties.get<Texture>("albedo");
    }

    // Evaluate the Lambertian BSDF for given directions
    BsdfEval evaluate(const Point2 &uv, const Vector &wo,
                      const Vector &wi) const override {        
        // std::cout << "\nwi: " << wi << std::endl;
        // std::cout << "\nwo: " << wo << std::endl;
        if (wo.z() * wi.z() < 0) {
            return BsdfEval::invalid();
        }

        float abs_costheta = Frame::absCosTheta(wi);
        Color albedo = m_albedo->evaluate(uv);

        // Return the costheta Lambertian BSDF value:
        // cos(theta) * B(wi, wo)
        return BsdfEval{
            .value = albedo * InvPi * abs_costheta,
            // .pdf   = cosineHemispherePdf(wi.normalized())
            .pdf   = abs(wi.z()) * InvPi
        };
    }

    // Sample a direction over the hemisphere for Lambertian reflection
    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {
        

        // Generate a cosine-weighted random direction over the hemisphere
        Vector wi = squareToCosineHemisphere(rng.next2D());

        // Ensure that the directions are valid (wo and wi on the same side of the surface)
        if (wo.z() < 0) {
            wi.z() *= -1;
        }

        // Compute the PDF for cosine-weighted hemisphere sampling
        float pdf = Frame::sameHemisphere(wo, wi) ? Frame::absCosTheta(wi) * InvPi : 0;

        // Lambertian BSDF value: B(wi, wo) = albedo / pi
        // weight = cos(theta) * B(wi, wo) / p(wi)
        // Color weight = evaluate(uv, wo, wi).value / pdf;
        Color weight = m_albedo->evaluate(uv);

        return BsdfSample{
            .wi = wi.normalized(),
            .weight = weight,
            .pdf = pdf
        };
    }

    // BsdfSample sample(const Point2 &uv, const Vector &wo,
    //                   Sampler &rng) const override {
        

    //     // Generate a cosine-weighted random direction over the hemisphere
    //     Vector wi = squareToCosineHemisphere(rng.next2D());

    //     // Ensure that the directions are valid (wo and wi on the same side of the surface)
    //     if (wo.z() < 0) {
    //         wi.z() *= -1;
    //     }

    //     // Compute the PDF for cosine-weighted hemisphere sampling
    //     float pdf = cosineHemispherePdf(wi); // p(wi) = cos(theta) / pi
    //     if (pdf <= 0) {
    //         return BsdfSample::invalid();
    //     }

    //     // Lambertian BSDF value: B(wi, wo) = albedo / pi
    //     // weight = cos(theta) * B(wi, wo) / p(wi)
    //     // Color weight = evaluate(uv, wo, wi).value / pdf;
    //     Color weight = m_albedo->evaluate(uv);

    //     return BsdfSample{
    //         .wi = wi.normalized(),
    //         .weight = weight,
    //         .pdf = pdf,
    //         .eta = 1.0f
    //     };
    // }


    Color getAlbedo(const Intersection &its) const override {
            return m_albedo->evaluate(its.uv);
        }


    std::string toString() const override {
        return tfm::format(
            "Diffuse[\n"
            "  albedo = %s\n"
            "]",
            indent(m_albedo));
    }

};

} // namespace lightwave

REGISTER_BSDF(Diffuse, "diffuse")