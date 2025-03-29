#include <lightwave.hpp>

namespace lightwave {

    class PrincipledVolume : public Bsdf {
        float m_absorption;
        float m_phase;
        ref<Texture> m_baseColor;

    public:
        PrincipledVolume(const Properties &properties) {
            m_baseColor  = properties.get<Texture>("baseColor");
            m_absorption = properties.get<float>("absorption");
            m_phase      = properties.get<float>("phase", 0);
        }

        BsdfEval evaluate(const Point2 &uv, const Vector &wo, const Vector &wi) const override {
            const auto m_color = m_baseColor->evaluate(uv);
            const float phasePdf = phase(m_phase, (-wo).dot(wi));
            return { m_color * phasePdf * m_absorption, 0.f};
        }

        BsdfSample sample(const Point2 &uv, const Vector &wo, Sampler &rng) const override {
            const auto m_color = m_baseColor->evaluate(uv);
            const Vector up = Vector(0, 0, 1); // because normal is random
            const float phasePdf = phase(m_phase, (-wo).dot(up));
            return BsdfSample(up, m_color * phasePdf * m_absorption, 0.f, 1.0f);
        }

        Color getAlbedo(const Intersection &its) const override {
            return m_baseColor->evaluate(its.uv);
        }

        float phase(float g, float cos_theta) const
        {
            float denom = 1 + g * g - 2 * g * cos_theta;
            return 1 / (4 * Pi) * (1 - g * g) / (denom * sqrtf(denom));
        }

        std::string toString() const override {
            return tfm::format(
                    "PrincipledVolume[\n"
                    "  absorption = %s,\n"
                    "]",
                    indent(m_absorption)
            );
        }
    };

}

REGISTER_BSDF(PrincipledVolume, "volume")

// --- back up here ----
// #include <lightwave.hpp>

// namespace lightwave {

//     class PrincipledVolume : public Bsdf {
//         float m_absorption;
//         float m_phase;
//         Color m_color;

//     public:
//         PrincipledVolume(const Properties &properties) {
//             m_color = properties.get<Color>("color", Color(1));
//             m_absorption = properties.get<float>("absorption");
//             m_phase = properties.get<float>("phase", 0);
//         }

//         BsdfEval evaluate(const Point2 &uv, const Vector &wo, const Vector &wi) const override {
//             const float phasePdf = phase(m_phase, (wo).dot(wi));

//             return BsdfEval{ 
//                         .value = m_color * phasePdf * m_absorption, 
//                         .pdf   = 0.f};
//         }

//         BsdfSample sample(const Point2 &uv, const Vector &wo, Sampler &rng) const override {
//             // HenyeyGreenstein sample
//             float costheta;
//             if (abs(m_phase) < Epsilon){
//                 costheta = 2 * rng.next() - 1;
//             } else {
//                 float temp = (1 - sqr(m_phase)) / (1 + m_phase - 2 * m_phase * rng.next());
//                 costheta   = - (1 + sqr(m_phase) - sqr(temp)) / (2 * m_phase);
//             }
//             float sintheta = sqrt(max(0.0f, 1 - sqr(costheta)));
//             float phi = 2 * Pi * rng.next();
//             Vector wi = Vector(sintheta * cos(phi), sintheta * sin(phi), costheta).normalized();
//             // std::cout << "costheta: " << costheta << ", wi: " << wi << std::endl;
//             const float phasePdf = phase(m_phase, (wo).dot(wi));
            
//             return BsdfSample{
//                     .wi = wi, 
//                     .weight = m_color * m_absorption * phasePdf, 
//                     .pdf = 0.f};
//         }

//         Color getAlbedo(const Intersection &its) const override {
//             return m_color;
//         }

//         float phase(float g, float cos_theta) const
//         {
//             float denom = 1 + g * g + 2 * g * cos_theta;
//             return Inv4Pi * (1 - g * g) / (denom * sqrt(denom));
//         }

//         // BsdfEval evaluate(const Point2 &uv, const Vector &wo, const Vector &wi) const override {
//         //     const float phasePdf = phase(m_phase, (-wo).dot(wi));
//         //     return { m_color * phasePdf * m_absorption, 0.f};
//         // }

//         // BsdfSample sample(const Point2 &uv, const Vector &wo, Sampler &rng) const override {
//         //     const Vector up = Vector(0, 0, 1); // because normal is random
//         //     const float phasePdf = phase(m_phase, (-wo).dot(up));
//         //     return BsdfSample(up, m_color * phasePdf * m_absorption, 0.f);
//         // }

//         // Color getAlbedo(const Intersection &its) const override {
//         //     return m_color;
//         // }

//         // float phase(float g, float cos_theta) const
//         // {
//         //     float denom = 1 + g * g - 2 * g * cos_theta;
//         //     return 1 / (4 * Pi) * (1 - g * g) / (denom * sqrtf(denom));
//         // }

//         std::string toString() const override {
//             return tfm::format(
//                     "PrincipledVolume[\n"
//                     "  absorption = %s,\n"
//                     "]",
//                     indent(m_absorption)
//             );
//         }
//     };

// }

// REGISTER_BSDF(PrincipledVolume, "volume")
