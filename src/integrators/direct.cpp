#include <lightwave.hpp>
// #include <fstream>

// std::ofstream logFile("log.txt", std::ios::app);

namespace lightwave {

class DirectLightIntegrator final : public SamplingIntegrator {
public:
    DirectLightIntegrator(const Properties &properties)
        : SamplingIntegrator(properties) {
        // No additional properties are needed; everything is initialized by SamplingIntegrator
    }

    /**
     * @brief Estimates the radiance along the given ray using direct illumination.
     * 
     * @param ray The ray to trace into the scene.
     * @param rng The random number generator for sampling.
     * @return The computed radiance.
     */
    Color Li(const Ray &ray, Sampler &rng) override {
        Color directLight = Color(0.0f);
        // Step a: Find the first intersection
        Intersection its = m_scene->intersect(ray.normalized(), rng);

        EmissionEval eval = its.evaluateEmission();
        directLight += Color(eval.value);
        if (!its) {
            // If no surface interaction, use the environment map (background light)
            return directLight;
        }

        ///BSDF sample
        const BsdfSample bsdf_sample = its.sampleBsdf(rng);
        if (!bsdf_sample.isInvalid()){
            Ray bsdf_ray = Ray(its.position, bsdf_sample.wi, 1).normalized();
            Intersection bsdfIts = m_scene->intersect(bsdf_ray, rng);
            EmissionEval bsdfEmission = bsdfIts.evaluateEmission();
            directLight += bsdf_sample.weight * Color(bsdfEmission.value);
        }

        //Light sources
        if (m_scene->hasLights()){
            // light sample
            const LightSample lightsample = m_scene->sampleLight(rng);
            if (!lightsample || !lightsample.light) {
                return directLight; // No valid light sample available
            }

            // Get direct light
            DirectLightSample directSample = lightsample.light->sampleDirect(its.position, rng);
            if (directSample.isInvalid()) {
                return directLight;
            }
            
            // move the ray towards the point light a bit to avoid self-intersection
            Ray ShadowRay(its.position, directSample.wi);
            ShadowRay = ShadowRay.normalized();
            if (m_scene->intersect(ShadowRay, directSample.distance, rng)) {
                return directLight; // Light is occluded, no contribution
            }

            // Compute light contribution with BSDF
            Color fr_cos = its.evaluateBsdf(directSample.wi).value;
            // float light_num = lightsample.probability > 0 ? 1 / lightsample.probability : 1.f;
            // std::cout << "\nE: " << directSample.weight*directSample.pdf << " ,pdf: " << directSample.pdf 
            //           << ", output: " << fr_cos * directSample.weight / lightsample.probability << std::endl;
            directLight += fr_cos * directSample.weight / lightsample.probability;
        }

        return directLight;
    }
        

    std::string toString() const override {
        return "DirectLightIntegrator[]";
    }
};
}
REGISTER_INTEGRATOR(DirectLightIntegrator, "direct")


    //     Color Li(const Ray &ray, Sampler &rng) override {
    //         const int DEPTH = 2;
    //         Color Li = Color(0.0f);
    //         Color weight = Color(1.0f);
    //         Ray currentRay = ray.normalized();

    //         for (int i = 0; i < DEPTH; i++) {
    //             // Step a: Find the first intersection
    //             Intersection its = m_scene->intersect(currentRay, rng);
    //             EmissionEval eval = its.evaluateEmission();
    //             Li += Color(eval.value) * weight;
                
    //             if (!its || i == DEPTH - 1) {
    //                 // If no surface interaction, use the environment map (background light)
    //                 return Li;
    //             }
                
    //             //Light sources
    //             if (m_scene->hasLights()){
    //                 const LightSample lightsample = m_scene->sampleLight(rng);
    //                 if (lightsample && lightsample.light) {
    //                     DirectLightSample directSample = lightsample.light->sampleDirect(its.position, rng);
    //                     if (!directSample.isInvalid()) {
    //                         // move the ray towards the point light a bit to avoid self-intersection
    //                         Ray ShadowRay(its.position, directSample.wi);
    //                         ShadowRay = ShadowRay.normalized();
    //                         if (!m_scene->intersect(ShadowRay, directSample.distance, rng)) {
    //                             // Light is occluded, no contribution
    //                             // Compute light contribution with BSDF
    //                             Color fr_cos = (its.evaluateBsdf(ShadowRay.direction)).value;
    //                             Li += (fr_cos * directSample.weight / lightsample.probability) * weight;
    //                         }
    //                     }
    //                 }
    //             }

    //             //BSDF sample
    //             const BsdfSample bsdf_sample = its.sampleBsdf(rng);
    //             if (!bsdf_sample.isInvalid()){
    //                 currentRay = Ray(its.position, bsdf_sample.wi).normalized();
    //                 weight *= bsdf_sample.weight;
    //             } else{
    //                 break;
    //             }
    //         }
    //         return Li;
    // }