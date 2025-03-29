#include <lightwave.hpp>

namespace lightwave {

class AreaLight final : public Light {
    ref<Instance> m_instance;

public:
    AreaLight(const Properties &properties) : Light(properties) {
        m_instance     = properties.getChild<Instance>();
        m_instance->setLight(this);
    }

    DirectLightSample sampleDirect(const Point &origin,
                                   Sampler &rng) const override {
        // NOT_IMPLEMENTED        
        // const AreaSample sample = m_instance->sampleArea(rng);
        const AreaSample sample = m_instance->improvedSampleArea(origin, rng);
        /// no need to transform position to world space here 
        /// because we did it in Instance::sampleArea()
        auto [length, wi] = (sample.position - origin).lengthAndNormalized();
        if (sample.pdf == 0.f || length == 0.f) return DirectLightSample::invalid();

        Vector local_wi = sample.shadingFrame().toLocal(wi).normalized();
        const auto Emission = m_instance->emission()->evaluate(sample.uv, -local_wi);
        float cosThetaL = Frame::absCosTheta(local_wi);
        // area light is computed as: L(-w_i) * costheta_l / (pdf * d^2)
        Color weight =  Emission.value * cosThetaL / (sqr(length) * sample.pdf);

        return DirectLightSample{
            .wi = wi,
            .weight = weight,
            .distance = length,
            // .pdf    = sample.pdf
            .pdf = SurfaceAreaPDFToSolidAnglePDF(sample.pdf, length, sample.shadingFrame().normal, -wi)
        };

    }

    bool canBeIntersected() const override { return m_instance->isVisible(); }

    std::string toString() const override {
        return tfm::format(
            "AreaLight[\n"
            // "]");
            "  instance = %s\n"
            "]",
            indent(m_instance));
    }
};

} // namespace lightwave

REGISTER_LIGHT(AreaLight, "area")
