/**
 * @file light.hpp
 * @brief Contains the Light interface and related structures.
 */

#pragma once

#include <lightwave/color.hpp>
#include <lightwave/core.hpp>
#include <lightwave/emission.hpp>
#include <lightwave/math.hpp>

namespace lightwave {


/// @brief Names of the implemented lights
enum class LightType { Directional, Point, Envmap, AreaLight };         

/// @brief The result of sampling a light from a given query point using @ref
/// Light::sampleDirect .
struct DirectLightSample {
    /// @brief The direction vector, pointing from the query point towards the
    /// light.
    Vector wi;
    /// @brief The weight of the sample, given by @code Le(-wi) / p(wi) @endcode
    Color weight;
    /// @brief The distance from the query point to the sampled point on the
    /// light source.
    float distance;
    /// @brief The probability of sampling this light source.
    float pdf;

    /// @brief Return an invalid sample, used to denote that sampling has
    /// failed.
    static DirectLightSample invalid() {
        return {
            .wi     = Vector(),
            .weight = Color(),
            .distance = 0,
            .pdf    = 0.0f
        };
    }

    /// @brief Tests whether the sample is invalid (i.e., sampling has failed).
    bool isInvalid() const {
        return weight == Color(0);
    }
    explicit operator bool() const { return !isInvalid(); }
};

/**
 * @brief A light source that can be sampled for direct connections.
 * Some light sources can also be intersected by rays (e.g., area lights or the
 * background light), while others can only be connected to by sampling them
 * (e.g., point lights or directional lights).
 */
class Light : public Object {
protected:
    /**
     * @brief How likely this light will be selected when picking a random light
     * source. A value of zero indicates that this light should never be
     * sampled. The probability of sampling this light will be its weight
     * divided by the total weight of all light sources in the scene.
     */
    float m_samplingWeight;

public:
    Light(const Properties &properties) {
        m_samplingWeight = properties.get<float>("weight", 1.f);
    }

    /**
     * @brief How likely this light will be selected when picking a random light
     * source. A value of zero indicates that this light should never be
     * sampled. The probability of sampling this light will be its weight
     * divided by the total weight of all light sources in the scene.
     */
    float samplingWeight() const { return m_samplingWeight; }

    /**
     * @brief Samples a random point on the light source and computes its
     * emission and probability of sampling.
     * @param origin The light receiving point that the emission and probability
     * should be computed for.
     * @param rng A random number generator used to steer the sampling.
     */
    virtual DirectLightSample sampleDirect(const Point &origin,
                                           Sampler &rng) const = 0;

    /// @brief Returns whether this light source can be hit by rays (i.e., has
    /// an area that has been placed within the scene).
    virtual bool canBeIntersected() const { return false; }

    // /// @brief Returns the type of light source 
    // virtual LightType getLightType() const = 0;
};

/**
 * @brief Provides a background color when a ray escapes the scene (e.g., by
 * using an environment map). Conceptually, a background light is an emissive
 * sphere of infinite radius.
 */
class BackgroundLight : public Light {
public:
    BackgroundLight(const Properties &properties) : Light(properties) {}

    /**
     * @brief Queries the background emission for a given direction pointing
     * away from the scene. We do not need to know the origin of the ray, as the
     * entire scene collapses to a singular point when viewed from infinitely
     * far away (which is where the background light resides).
     * @param direction The direction in world coordinates, pointing away from
     * the scene.
     */
    virtual EmissionEval evaluate(const Vector &direction) const = 0;

    DirectLightSample sampleDirect(const Point &origin,
                                   Sampler &rng) const override {
        return DirectLightSample::invalid();
    }

    bool canBeIntersected() const override { return true; }
};

} // namespace lightwave
