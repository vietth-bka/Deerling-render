#pragma once

#include <lightwave/core.hpp>
#include <lightwave/math.hpp>
#include <lightwave/sampler.hpp>

namespace lightwave {

/// @brief Models a grid of volumetric densities
class Volume : public Object {
public:
    /**
     * @brief Returns the density at a given point
     */
    virtual float evaluate(const Point &pos) const = 0;
    virtual float getMaxDensity() const = 0;

    bool sampleDistance(float tMin, float insideT, float scaleFactor,
        const Ray &localRay, Sampler &rng, float &distance) const
    {
        // const int TRACKING_STEPS = 1024;
        const float maxDensity = getMaxDensity();
        float t = tMin;
        float tMax = tMin + insideT;
        // for (int i = 0; i < TRACKING_STEPS; i++) {
        while (true) {
            const float sigmaT = maxDensity;
            // Sample random distance in local space
            const float sampleT = -log(1 - rng.next()) / sigmaT * scaleFactor;

            if (t + sampleT < tMax) {
                // Sampled a point inside the volume
                t += sampleT;
                const float realProbability = evaluate(localRay(t)) / maxDensity;

                if (rng.next() < realProbability) {
                    // Convert nextT back to world space
                    distance = t / scaleFactor;
                    return true;
                } 
            } else {
                // Ignore intersection, sampeld outside of volume
                return false;
            }
        }

        // Not sure what to do here, ran out of tracking steps, volume too dense
        distance = t / scaleFactor;
        return true;
    }

};

}
