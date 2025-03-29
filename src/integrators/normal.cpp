#include <lightwave.hpp>

namespace lightwave {

class NormalIntegrator : public SamplingIntegrator {
    bool remap;

public:
    NormalIntegrator(const Properties &properties)
        : SamplingIntegrator(properties) {

        remap = properties.get<bool>("remap", true);    
    }

    /**
     * @brief The job of an integrator is to return a color for a ray produced
     * by the camera model. This will be run for each pixel of the image,
     * potentially with multiple samples for each pixel.
     */
    Color Li(const Ray &ray, Sampler &rng) override {
        Intersection its = m_scene->intersect(ray, rng);
        Vector normal    = its ? its.shadingNormal : Vector(0.5f, 0.5f, 0.5f);
        
        return this->remap ? Color((normal + Vector(1)) / 2) : Color(normal);
    }

    /// @brief An optional textual representation of this class, which can be
    /// useful for debugging.
    std::string toString() const override {
        return tfm::format("NormalIntegrator[\n"
                           "  sampler = %s,\n"
                           "  image = %s,\n"
                           "]",
                           indent(m_sampler), indent(m_image));
    }
};

} // namespace lightwave

REGISTER_INTEGRATOR(NormalIntegrator, "normal")
