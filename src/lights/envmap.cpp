#include <lightwave.hpp>
#include <chrono>

// #include <fstream>
// std::ofstream logFile("log.txt", std::ios::app);

namespace lightwave {

class EnvironmentMap final : public BackgroundLight {
    /// @brief The texture to use as background
    ref<Texture> m_texture;
    /// @brief An optional transform from local-to-world space
    ref<Transform> m_transform;

public:
    EnvironmentMap(const Properties &properties) : BackgroundLight(properties) {
        m_texture   = properties.getChild<Texture>();
        m_transform = properties.getOptionalChild<Transform>();
    }

    EmissionEval evaluate(const Vector &direction) const override {
        Point2 warped = Point2(0);
        // hints:
        // * if (m_transform) { transform direction vector from world to local
        // coordinates }
        // * find the corresponding pixel coordinate for the given local
        // direction
        // * make use of std::atan2 instead of tangent function.
        // * check out the safe versions of sine and cosine, e.g. safe_acos
        // in math.hpp to avoid problematic edge cases

        // Step 1: Transform direction to local space (if transform is defined)        
        Vector localDir = direction;
        if (m_transform) {
            localDir = m_transform->inverse(localDir);
        }

        // Step 2: Normalize the direction vector
        localDir = localDir.normalized();

        // // Step 3: Convert local direction to spherical coordinates (theta, phi)
        float phi = std::atan2(localDir.z(), localDir.x());   // phi in [-pi, pi], angle in the x-z plane
        float theta = safe_acos(localDir.y());              // theta in [0, pi], angle from the y-axis

        // // Step 4: Map spherical coordinates to texture coordinates (u, v)
        // // u (horizontal): Map phi from [-pi, pi] to [0, 1]
        // // v (vertical): Map theta from [0, pi] to [1, 0] (flip v)
        float u = 0.5f - phi / (2*Pi);
        // float v = 1.0f - (theta / Pi);
        float v = theta / Pi;

        warped = Point2(u, v);

        // Step 5: Evaluate the environment texture at the computed coordinates

        return {
            .value = m_texture->evaluate(warped),
            .pdf   = Inv4Pi
        };

    }

    DirectLightSample sampleDirect(const Point &origin,
                                   Sampler &rng) const override {
        Point2 warped    = rng.next2D();
        Vector direction = squareToUniformSphere(warped);
        auto E           = evaluate(direction);

        // implement better importance sampling here, if you ever need it
        // (useful for environment maps with bright tiny light sources, like the
        // sun for example)

        return {
            .wi = direction,
            .weight = E.value / Inv4Pi,
            .distance = Infinity,
            .pdf = Inv4Pi
        };
    }

    std::string toString() const override {
        return tfm::format(
            "EnvironmentMap[\n"
            "  texture = %s,\n"
            "  transform = %s\n"
            "]",
            indent(m_texture),
            indent(m_transform));
    }
};

} // namespace lightwave

REGISTER_LIGHT(EnvironmentMap, "envmap")
