#include <lightwave.hpp>
#include <fstream>


namespace lightwave {

class PathTracer final : public SamplingIntegrator {
    int DEPTH;

public:
    PathTracer(const Properties &properties)
        : SamplingIntegrator(properties) {
        DEPTH = properties.get<int>("depth", 2);
    }

    Color Li(const Ray &ray, Sampler &rng) override {
        Color Li = Color(0.0f);
        Color weight = Color(1.0f);
        Ray currentRay = ray.normalized();
        float etaScale = 1.0f;

        for (int i = 0; i < DEPTH; i++) {
            // Step a: Find the first intersection
            Intersection its = m_scene->intersect(currentRay, rng);
            EmissionEval eval = its.evaluateEmission();
            // if (i < int(DEPTH/2)) // newly added for noise reduction
            Li += Color(eval.value) * weight;
            
            if (!its || i == DEPTH - 1) {
                // If no surface interaction, use the environment map (background light)
                return Li;
            }
            
            //Light sources
            if (m_scene->hasLights()){
                const LightSample lightsample = m_scene->sampleLight(rng);
                if (lightsample && lightsample.light) {
                    DirectLightSample directSample = lightsample.light->sampleDirect(its.position, rng);
                    if (!directSample.isInvalid()) {
                        // move the ray towards the point light a bit to avoid self-intersection
                        Ray ShadowRay(its.position, directSample.wi);
                        ShadowRay = ShadowRay.normalized();
                        if (!m_scene->intersect(ShadowRay, directSample.distance, rng)) {
                            // Light is occluded, no contribution
                            // Compute light contribution with BSDF
                            Color fr_cos = (its.evaluateBsdf(ShadowRay.direction)).value;
                            Li += (fr_cos * directSample.weight / lightsample.probability) * weight;
                        }
                    }
                }
            }

            //BSDF sample
            const BsdfSample bsdf_sample = its.sampleBsdf(rng);
            if (!bsdf_sample.isInvalid()){
                currentRay = Ray(its.position, bsdf_sample.wi).normalized();
                // update variables
                weight *= bsdf_sample.weight;
                etaScale *= sqr(bsdf_sample.eta);
                
            } else{
                break;
            }

            /// Russian roulette
            // Color rrBeta = weight * etaScale;
            // float maxComponent = max(max(rrBeta[0], rrBeta[1]), rrBeta[2]);
            // if (i > 2 && maxComponent < 1)
            //     {
            //         float q = max(0.0f, 1.0f - maxComponent);
            //         if (rng.next() < q)
            //             break;
            //         if (q > 1.0f - Epsilon)
            //             q = 1.0f - Epsilon;
            //         weight /= (1 - q);
            //     }
        }
        return Li;
    }

    std::string toString() const override {
        return "PathTracer[]";
    }
};
}
REGISTER_INTEGRATOR(PathTracer, "pathtracer")


//------------------- old version with recursive path-tracing -------------------//

// namespace lightwave {

// class PathTracer final : public SamplingIntegrator {
//     int depth;

// public:
//     PathTracer(const Properties &properties)
//         : SamplingIntegrator(properties) {
//         depth = properties.get<int>("depth", 2);
//     }

//     Color Li(const Ray &ray, Sampler &rng) override {
//         Color directLight = Color(0.0f);                                
//         Intersection its = m_scene->intersect(ray.normalized(), rng);   // first hit
//         directLight += its.evaluateEmission().value;
//         if (!its){
//             return directLight;
//         }
//         directLight += FromLightSources(its, rng);                      // radiance from light source
//         directLight += RecurseRadiance(its, ray.depth, rng);            // recursively trace a ray

//         return directLight;

//     }

//     Color RecurseRadiance(const Intersection &its, const int init_depth, Sampler &rng){
//         Color radiance(0.0f);

//         BsdfSample bsdf_sample = its.sampleBsdf(rng);
//         if (!bsdf_sample.isInvalid()){
//             Ray ray = Ray(its.position, bsdf_sample.wi, init_depth + 1).normalized();
//             Intersection next_its = m_scene->intersect(ray, rng);
//             if (ray.depth >= depth - 1){                                                        // last hit
//                 radiance = bsdf_sample.weight * (next_its.evaluateEmission().value);
//                                                 // + FromLightSources(next_its, rng));
//                 return radiance;
//             }
//             radiance = bsdf_sample.weight * (next_its.evaluateEmission().value                  // Emission illuminum
//                                                 + FromLightSources(next_its, rng)               // Direct illuminum
//                                                 + RecurseRadiance(next_its, ray.depth, rng));   // Indirect illuminum
//         } 

//         return radiance;
//     }

//     // compute radiance from light sources as in direct.cpp
//     Color FromLightSources(const Intersection &its, Sampler &rng){
//         Color directLight(0.0f);

//         const LightSample lightsample = m_scene->sampleLight(rng);
//         if (!lightsample || !lightsample.light) {
//             return directLight; // No valid light sample available
//         }

//         // Get direct light
//         DirectLightSample directSample = lightsample.light->sampleDirect(its.position, rng);
//         if (directSample.isInvalid()) {
//             return directLight;
//         }
        
//         Ray ShadowRay(its.position + Epsilon * directSample.wi, directSample.wi);
//         ShadowRay = ShadowRay.normalized();
//         if (m_scene->intersect(ShadowRay, directSample.distance - Epsilon, rng)) {
//             return directLight; // Light is occluded, no contribution
//         }

//         // Compute light contribution with BSDF
//         Color f_r = (its.evaluateBsdf(ShadowRay.direction)).value;
//         float light_num = lightsample.probability > 0 ? 1 / lightsample.probability : 1.f;
//         directLight += directSample.weight * f_r * light_num;
        
//         return directLight;
//     }

//     std::string toString() const override {
//         return "PathTracer[]";
//     }
// };
// }
// REGISTER_INTEGRATOR(PathTracer, "pathtracer")