#include <lightwave/camera.hpp>
#include <lightwave/core.hpp>
#include <lightwave/integrator.hpp>
#include <lightwave/light.hpp>
#include <lightwave/registry.hpp>
#include <lightwave/shape.hpp>
#include <lightwave/instance.hpp>
#include <lightwave/profiler.hpp>

#include <unordered_map>

namespace lightwave {

class Scene::LightSampling {
    struct DistributionElement {
        const Light *light;
        float probability;
        float cdf;
    };

    /// @brief References to all lights, to maintain memory ownership.
    std::vector<ref<Light>> m_lights;
    /// @brief A fast lookup table to query how likely a light is to be sampled.
    std::unordered_map<const Light *, float> m_probabilities;
    /// @brief The distribution used for sampling.
    std::vector<DistributionElement> m_distribution;

public:
    LightSampling(const std::vector<ref<Light>> &lights)
    : m_lights(lights) {
        float cummulativeWeight = 0;

        for (const auto &light : lights) {
            const float weight = light->samplingWeight();
            if (weight == 0) {
                // this light does not want to be sampled, so do not add
                // it to the distribution and set its probability to 0
                m_probabilities.insert(std::make_pair(light.get(), 0));
                continue;
            }

            cummulativeWeight += weight;
            m_distribution.emplace_back(DistributionElement {
                light.get(),
                weight,
                cummulativeWeight
            });
        }

        // normalize the distribution so that probability and cdf are in the range [0,1]
        for (auto &element : m_distribution) {
            element.probability /= cummulativeWeight;
            element.cdf /= cummulativeWeight;

            // add light to lookup table so that the probability can be queried efficiently
            m_probabilities[element.light] = element.probability;
        }
    }

    bool hasLights() const { return !m_lights.empty(); }

    LightSample sample(Sampler &rng) const {
        if (m_distribution.empty()) return LightSample::invalid();
        const auto cdf = rng.next();
        const auto element = std::upper_bound(
            m_distribution.begin(),
            m_distribution.end(),
            cdf,
            [](float value, const DistributionElement &element) {
                return value < element.cdf;
            }
        );
        return {
            .light = element->light,
            .probability = element->probability,
        };
    }

    float probability(const Light *light) const {
        if (light == nullptr) return 0;

        const auto it = m_probabilities.find(light);
        if (it == m_probabilities.end()) return 0;
        return it->second;
    }

    float get_mlights_size() {  //newly added
        return float(m_lights.size());
    }
};

Scene::Scene(const Properties &properties) {
    m_camera     = properties.getChild<Camera>();
    m_background = properties.getOptionalChild<BackgroundLight>();
    m_lightSampling = std::make_shared<LightSampling>(properties.getChildren<Light>());

    const std::vector<ref<Shape>> entities = properties.getChildren<Shape>();
    if (entities.size() == 1) {
        m_shape = entities[0];
    } else {
        m_shape = std::static_pointer_cast<Shape>(
            Registry::create("shape", "group", properties));
    }

    m_shape->markAsVisible();
}

std::string Scene::toString() const {
    return tfm::format("Scene[\n"
                       "  camera = %s,\n"
                       "  shape = %s,\n"
                       "]",
                       indent(m_camera), indent(m_shape));
}

Intersection Scene::intersect(const Ray &ray, Sampler &rng) const {
    PROFILE("Intersect")

    Intersection its(-ray.direction);
    m_shape->intersect(ray, its, rng);
    if (!its) {
        its.background = m_background.get();
    }
    its.lightProbability = m_lightSampling->probability(its.light());
    return its;
}

bool Scene::intersect(const Ray &ray, float tMax, Sampler &rng) const {
    PROFILE("Shadow ray")

    Intersection its(-ray.direction, tMax * (1 - Epsilon));
    return m_shape->intersect(ray, its, rng);
}

LightSample Scene::sampleLight(Sampler &rng) const {
    PROFILE("Pick light")

    return m_lightSampling->sample(rng);
}

float Scene::lightSelectionProbability(const Light *light) const { //newly added
    return float(1) / m_lightSampling->get_mlights_size();
}

bool Scene::hasLights() const {
    return m_lightSampling->hasLights();
}

Bounds Scene::getBoundingBox() const { return m_shape->getBoundingBox(); }

} // namespace lightwave

REGISTER_CLASS(Scene, "scene", "default")
