#include <lightwave.hpp>
#include <ostream>
#include <lightwave/instance.hpp>
// #include <OpenImageDenoise/oidn.hpp>


namespace lightwave {

class MISPathTracer final : public SamplingIntegrator {
    int DEPTH;

public:
    MISPathTracer(const Properties &properties)
        : SamplingIntegrator(properties) {
        DEPTH = properties.get<int>("depth", 2);
    }

    Color Li(const Ray &ray, Sampler &rng) override {
        Color Li = Color(0.0f);
        Color weight = Color(1.0f);
        float p_bsdf = 1.0f;
        // float etaScale = 1.0f;
        Ray currentRay = ray.normalized();

        for (int i = 0; i < DEPTH; i++) {
            // Find the first intersection
            Intersection its = m_scene->intersect(currentRay, rng);
            EmissionEval eval = its.evaluateEmission();
                        

            // If no intersection, check if it is the first intersection or 
            // there is an infinite light source
            if (!its){
                if (i == 0){
                    Li += Color(eval.value) * weight;
                } else if (its.background != nullptr){ // intersected with the environment map - infinite light source
                    float w_b = 1.0f;
                    if (its.background->canBeIntersected()){
                        float p_light = eval.pdf * its.lightProbability;
                        w_b = PowerHeuristic(1, p_bsdf, 1, p_light);
                        // std::cout << "\np_bsdf: " << p_bsdf << ", p_light: " << p_light << ", w_b: " << w_b << std::endl;
                    }
                    Li += Color(eval.value) * w_b * weight;
                }

                return Li;
                
            }
            // If there exists an intersection, add the emission value at 
            // the first intersection point, otherwise add the emission value times MIS weight
            if (i == 0 || its.instance->light() == nullptr)
            // if (i == 0)
            {
                Li += Color(eval.value) * weight;
                // if (i != 0 && its.instance->emission()) 
                //     std::cout<<"here1" <<std::endl;
            }
            else if (its.instance->light() != nullptr) {                
                float p_light = SurfaceAreaPDFToSolidAnglePDF(its.pdf, its.t, its.shadingFrame().normal, its.wo) * its.lightProbability;
                float w_b = PowerHeuristic(1, p_bsdf, 1, p_light);
                Li += Color(eval.value) * w_b * weight;
                // std::cout<<"here2" <<std::endl;
            }

            if (i == DEPTH - 1)
                return Li;
                        
            //Light sources
            if (m_scene->hasLights()){
                const LightSample lightsample = m_scene->sampleLight(rng);
                if (lightsample && lightsample.light) {
                    DirectLightSample directSample = lightsample.light->sampleDirect(its.position, rng);
                    if (!directSample.isInvalid()) {
                        // move the ray towards the point light a bit to avoid self-intersection
                        Ray ShadowRay = Ray(its.position, directSample.wi).normalized();
                        if (!m_scene->intersect(ShadowRay, directSample.distance, rng)) {
                            // Light is occluded, no contribution
                            // Compute light contribution with BSDF
                            BsdfEval bsdfeval = its.evaluateBsdf(ShadowRay.direction);
                            Color fr_cos    = bsdfeval.value;
                            float w_l = 1.0f;
                            if (lightsample.light->canBeIntersected()) {
                                float p_bsdf    = bsdfeval.pdf;
                                float p_light   = directSample.pdf * lightsample.probability; // need to check if the light can be intersected or not
                                w_l    = PowerHeuristic(1, p_light, 1, p_bsdf);
                                // std::cout << "\np_light: " << p_light << ", p_bsdf: " << p_bsdf 
                                // << ", w_l: " << w_l << ", light p: " << lightsample.probability << std::endl;
                                }
                            Li += (fr_cos * directSample.weight / lightsample.probability * w_l) * weight;
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
                p_bsdf  = bsdf_sample.pdf;
                // etaScale *= sqr(bsdf_sample.eta);

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
    
    float PowerHeuristic(int nf, float fPdf, int ng, float gPdf) {
        fPdf = clamp(fPdf, Epsilon, Infinity);
        gPdf = clamp(gPdf, Epsilon, Infinity);
        
        if (fPdf == Infinity)
            return 1.0f;
        else if (gPdf == Infinity)
            return 0.0f;
        
        float f = nf * fPdf, g = ng * gPdf;
        return sqr(f) / (sqr(f) + sqr(g));
    }

    std::string toString() const override {
        return "MISPathTracer[]";
    }
};
}
REGISTER_INTEGRATOR(MISPathTracer, "mis")


//-------save here for back up-------//
// #include <lightwave.hpp>
// #include <ostream>
// #include <lightwave/instance.hpp>
// #include <OpenImageDenoise/oidn.hpp>


// namespace lightwave {

// class MISPathTracer final : public SamplingIntegrator {
//     int DEPTH;

// public:
//     MISPathTracer(const Properties &properties)
//         : SamplingIntegrator(properties) {
//         DEPTH = properties.get<int>("depth", 2);
//     }

//     Color Li(const Ray &ray, Sampler &rng) override {
//         Color Li = Color(0.0f);
//         Color weight = Color(1.0f);
//         float p_bsdf, etaScale = 1.0f;
//         Ray currentRay = ray.normalized();
//         // float lightSelectionProb = m_scene->lightSelectionProbability(nullptr);

//         for (int i = 0; i < DEPTH; i++) {
//             // Find the first intersection
//             Intersection its = m_scene->intersect(currentRay, rng);
//             EmissionEval eval = its.evaluateEmission();
                        

//             // If no intersection, check if it is the first intersection or 
//             // there is an infinite light source
//             if (!its){
//                 if (i == 0){
//                     Li += Color(eval.value) * weight;
//                 } else if (its.background != nullptr){ // intersected with the environment map - infinite light source
//                     float p_light = eval.pdf * its.lightProbability; // may remove lightProbability here with more light sources
//                     float w_b = PowerHeuristic(1, p_bsdf, 1, p_light);
//                     // std::cout << "\nw_b: " << w_b << ", prob b: " << p_bsdf << ", prob l: " << p_light << std::endl;
//                     // std::cout << "\nEmission pdf: " << eval.pdf << std::endl;
//                     // std::cout << "\nlightP: " << its.lightProbability << std::endl;
//                     Li += Color(eval.value) * w_b * weight;
//                 }

//                 return Li;
                
//             }
//             // If there exists an intersection, add the emission value at 
//             // the first intersection point, otherwise add the emission value times MIS weight
//             if (i == 0)
//                 Li += Color(eval.value) * weight;
//             else if (its.instance->light() != nullptr) {
//                 // float p_light = its.pdf * its.lightProbability;
//                 float p_light = SurfaceAreaPDFToSolidAnglePDF(its.pdf, its.t, its.shadingFrame().normal, its.wo) * its.lightProbability;
//                 float w_b = PowerHeuristic(1, p_bsdf, 1, p_light);
//                 Li += Color(eval.value) * w_b * weight;
//             }            

//             if (i == DEPTH - 1)
//                 // If no surface interaction, use the environment map (background light)
//                 return Li;       

            
//             //Light sources
//             if (m_scene->hasLights()){
//                 const LightSample lightsample = m_scene->sampleLight(rng);
//                 if (lightsample && lightsample.light) {
//                     DirectLightSample directSample = lightsample.light->sampleDirect(its.position, rng);
//                     if (!directSample.isInvalid()) {
//                         // move the ray towards the point light a bit to avoid self-intersection
//                         Ray ShadowRay = Ray(its.position, directSample.wi).normalized();
//                         if (!m_scene->intersect(ShadowRay, directSample.distance, rng)) {
//                             // Light is occluded, no contribution
//                             // Compute light contribution with BSDF
//                             BsdfEval bsdfeval = its.evaluateBsdf(ShadowRay.direction);
//                             Color fr_cos = bsdfeval.value;
//                             //float p_bsdf    = bsdfeval.pdf;
//                             //float p_light   = directSample.pdf * lightsample.probability;
//                             //float w_l    = PowerHeuristic(1, p_light, 1, p_bsdf);
//                             float w_l = 1.0f;
//                             if (lightsample.light->canBeIntersected()) {
//                                float p_bsdf    = bsdfeval.pdf;
//                                float p_light   = directSample.pdf * lightsample.probability; // need to check if the light can be intersected or not
//                                w_l    = PowerHeuristic(1, p_light, 1, p_bsdf);
//                               }
//                             Li += (fr_cos * directSample.weight / lightsample.probability * w_l) * weight;
//                         }
//                     }
//                 }
//             }

//             //BSDF sample
//             const BsdfSample bsdf_sample = its.sampleBsdf(rng);
//             if (!bsdf_sample.isInvalid()){
//                 currentRay = Ray(its.position, bsdf_sample.wi).normalized();
//                 // update variables
//                 weight *= bsdf_sample.weight;
//                 p_bsdf  = bsdf_sample.pdf;
//                 etaScale *= sqr(bsdf_sample.eta);

//             } else{
//                 break;
//             }

//             /// Russian roulette
//             Color rrBeta = weight * etaScale;
//             float maxComponent = max(max(rrBeta[0], rrBeta[1]), rrBeta[2]);
//             if (i > 2 && maxComponent < 1)
//                 {
//                     float q = max(0.0f, 1.0f - maxComponent);
//                     if (rng.next() < q)
//                         break;
//                     if (q > 1.0f - Epsilon)
//                         q = 1.0f - Epsilon;
//                     weight /= (1 - q);
//                 }

//         }
//         return Li;
//     }
    
//     float PowerHeuristic(int nf, float fPdf, int ng, float gPdf) {
//         fPdf = clamp(fPdf, Epsilon, Infinity);
//         gPdf = clamp(gPdf, Epsilon, Infinity);
        
//         if (fPdf == Infinity)
//             return 1.0f;
//         else if (gPdf == Infinity)
//             return 0.0f;
        
//         float f = nf * fPdf, g = ng * gPdf;
//         return sqr(f) / (sqr(f) + sqr(g));
//     }

//     std::string toString() const override {
//         return "MISPathTracer[]";
//     }
// };
// }
// REGISTER_INTEGRATOR(MISPathTracer, "mis")