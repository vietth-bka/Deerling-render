#include <lightwave.hpp>
#include <vector>
#include <numeric>


namespace lightwave {


struct Distribution1D{
        std::vector<float> func, cdf;
        float funcInt;

        Distribution1D(std::vector<float> &f) : func(f) {            
            int n = func.size();
            cdf.resize(n + 1);
            cdf[0] = 0;

            for (int i = 1; i <= n; ++i ){
                cdf[i] = cdf[i-1] + func[i-1] / n;    
            }

            funcInt = cdf[n];
            if (funcInt == 0){
                for (int i=1; i <=n; ++i){
                    cdf[i] = float(i) / float(n);
                }
            } else {
                for (int i=1; i <=n; ++i){
                    cdf[i] /= funcInt;
                }
            }
            // std::cout << "CDF: " << cdf[10] << std::endl;
        }

        int count() const {
            return func.size();
        }

        float sampleContinuous(float u, float *pdf, int *off = nullptr) const{
            int offset = std::lower_bound(cdf.begin(), cdf.end(), u) - cdf.begin() - 1;
            offset     = max(0, offset);
            if (off) *off = offset;
            float du = (u - cdf.at(offset));
            if (cdf[offset + 1] - cdf[offset] > 0){
                du /= cdf[offset + 1] - cdf[offset];
            }
            if (pdf) *pdf = func.at(offset) / funcInt;
            // std::cout << "pdf: " << *pdf << std::endl;
            
            return (offset + du)/count(); // return sample value
        }

    };

class Distribution2D {
public:
    std::vector<std::unique_ptr<Distribution1D>> pConditionalV;
    std::unique_ptr<Distribution1D> pMarginal;

    Distribution2D(const std::vector<float> data, int m_width, int m_height) {
        for (int v = 0; v < m_height; ++v) {
            std::vector<float> conditionalFunc(data.begin() + v * m_width, data.begin() + (v + 1) * m_width);
            pConditionalV.push_back(std::make_unique<Distribution1D>(conditionalFunc));
        }
        
        std::vector<float> marginalFunc;
        for (const auto &dist: pConditionalV){
            marginalFunc.push_back(dist->funcInt);
        }
        pMarginal = std::make_unique<Distribution1D>(marginalFunc);
        
    }

    Point2 sampleContinuous(const Point2 &u, float *pdf) const {
        float pdfs[2];
        int v;
        float d1 = pMarginal->sampleContinuous(u.y(), &pdfs[1], &v);
        float d0 = pConditionalV[v]->sampleContinuous(u.x(), &pdfs[0]);
        *pdf = pdfs[0] * pdfs[1];
        return Point2(d0, d1);
    }

    float pdf(const Point2 &p) const {
        int iu = std::clamp(int(p.x() * pConditionalV[0]->count()), 0, pConditionalV[0]->count() - 1);
        int iv = std::clamp(int(p.y() * pMarginal->count()), 0, pMarginal->count() - 1);
        return pConditionalV[iv]->func[iu] / pMarginal->funcInt;
    }
};

class ImprovedEnvironmentMap final : public BackgroundLight {
    /// @brief The texture to use as background
    ref<Texture> m_texture;
    /// @brief An optional transform from local-to-world space
    ref<Transform> m_transform;

    bool importanceSampling;
    float m_width, m_height;

    std::unique_ptr<Distribution2D> m_distribution;

public:
    ImprovedEnvironmentMap(const Properties &properties) : BackgroundLight(properties) {
        m_texture   = properties.getChild<Texture>();
        m_transform = properties.getOptionalChild<Transform>();
        importanceSampling = properties.get<bool>("importanceSampling", false);
        std::cout << "\nImportance Sampling: " << importanceSampling << std::endl;

        std::shared_ptr<ImageTexture> imageTexture = std::dynamic_pointer_cast<ImageTexture>(m_texture);
        if (imageTexture && importanceSampling){
            Point2i m_resolution = imageTexture->m_image->resolution();
            std::cout << "\nImage resolution: " << m_resolution << std::endl;
            m_width = m_resolution.x();
            m_height = m_resolution.y();

            std::vector<float> imgLuminance(m_width * m_height);
            for (int v = 0; v < m_height; ++v){            
                float vp = float(v) / float(m_height);
                float sinTheta = sin(Pi * float(v + 0.5f) / float(m_height));
                for (int u = 0; u < m_width; ++u){
                    float up = float(u) / float(m_width);
                    imgLuminance[v * m_width + u] = m_texture->evaluate(Point2(up, vp)).luminance() * sinTheta;
                    // imgLuminance[v * m_width + u] = m_texture->scalar(Point2(up, vp)) * sinTheta;
                }
            }
            m_distribution = std::make_unique<Distribution2D>(imgLuminance, m_width, m_height);
        }
    }

    EmissionEval evaluate(const Vector &direction) const override {
        // hints:
        // * if (m_transform) { transform direction vector from world to local
        // coordinates }
        // * find the corresponding pixel coordinate for the given local
        // direction
        // * make use of std::atan2 instead of tangent function.
        // * check out the safe versions of sine and cosine, e.g. safe_acos
        // in math.hpp to avoid problematic edge cases
   
        Vector localDir = direction;
        if (m_transform) {
            localDir = m_transform->inverse(localDir);
        }

        localDir = localDir.normalized();
        float phi = std::atan2(localDir.z(), localDir.x());   // phi in [-pi, pi], angle in the x-z plane
        float theta = safe_acos(localDir.y());              // theta in [0, pi], angle from the y-axis

        // Map spherical coordinates to texture coordinates (u, v)
        // u (horizontal): Map phi from [-pi, pi] to [0, 1]
        // v (vertical): Map theta from [0, pi] to [1, 0] (flip v)
        float u = 0.5f - phi / (2*Pi);
        float v = theta / Pi;

        Point2 warped(u, v);
        float pdf = (m_distribution && importanceSampling) ? m_distribution->pdf(warped) / (2 * sqr(Pi) * sin(theta)) : Inv4Pi;

        return {
            .value = m_texture->evaluate(warped),
            .pdf   = pdf
        };

    }

    DirectLightSample sampleDirect(const Point &origin,
                                   Sampler &rng) const override {        
        // implement better importance sampling here, if you ever need it
        // (useful for environment maps with bright tiny light sources, like the
        // sun for example)
        if (!importanceSampling || !m_distribution){
            Point2 warped    = rng.next2D();
            Vector direction = squareToUniformSphere(warped);
            if (m_transform) 
                direction = m_transform->apply(direction).normalized();
            auto E           = evaluate(direction);

            return {
                .wi = direction,
                .weight = E.value / Inv4Pi,                
                .distance = Infinity,
                .pdf = Inv4Pi
            };
        }

        float mapPDF;
        Point2 uv = m_distribution->sampleContinuous(rng.next2D(), &mapPDF);
        // std::cout << "mapPDF: " << mapPDF << std::endl;
        if (mapPDF == 0){
            return DirectLightSample::invalid();
        }
        float theta = uv.y() * Pi;
        float phi   = (1 - 2 * uv.x()) * Pi;
        Vector wi   = Vector(cos(phi) * sin(theta),
                             cos(theta),
                             sin(phi) * sin(theta));
        if (m_transform) 
            wi = m_transform->apply(wi).normalized();

        float pdf = sin(theta) == 0 ? 0 : mapPDF / (2 * sqr(Pi) * sin(theta));
        
        if (pdf == 0.0f)
            return DirectLightSample::invalid();
        
        auto E = m_texture->evaluate(uv);

        return {
            .wi = wi,
            .weight = E / pdf,
            .distance = Infinity,
            .pdf = pdf
        };
    }

    std::string toString() const override {
        return tfm::format(
            "ImprovedEnvironmentMap[\n"
            "  texture = %s,\n"
            "  transform = %s\n"
            "]",
            indent(m_texture),
            indent(m_transform));
    }
};

} // namespace lightwave

REGISTER_LIGHT(ImprovedEnvironmentMap, "envmapIS")