#include <lightwave/core.hpp>
#include <lightwave/instance.hpp>
#include <lightwave/registry.hpp>
#include <lightwave/sampler.hpp>
#include <lightwave/warp.hpp> 

namespace lightwave {

void Instance::transformFrame(SurfaceEvent &surf, const Vector &wo) const {
    surf.position       = m_transform->apply(surf.position);

    Vector bitangent    = surf.shadingFrame().bitangent;
    Vector world_tangent= m_transform->apply(surf.tangent);
    // compute the real pA which should be 1/(4pi*r^2) for the sphere, r is the 'scale' in xml
    surf.pdf            /= world_tangent.cross(m_transform->apply(bitangent)).length();
    surf.tangent        = world_tangent.normalized();
    surf.geometryNormal = m_transform->applyNormal(surf.geometryNormal).normalized();
    surf.shadingNormal  = m_transform->applyNormal(surf.shadingNormal).normalized();
    
    if (m_normal) { // newly added shading normal 
        Vector normalMap = (2.0f * m_normal->evaluate(surf.uv) - Color(1.0f)).data();
        // update shading normal of surface event
        surf.shadingNormal = (surf.shadingFrame().toWorld(normalMap)).normalized();
        // update tangent according to the new shading normal
        surf.tangent = surf.shadingFrame().tangent.normalized();
    }

}

inline void validateIntersection(const Intersection &its) {
    // use the following macros to make debugginer easier:
    // * assert_condition(condition, { ... });
    // * assert_normalized(vector, { ... });
    // * assert_ortoghonal(vec1, vec2, { ... });
    // * assert_finite(value or vector or color, { ... });

    // each assert statement takes a block of code to execute when it fails
    // (useful for printing out variables to narrow done what failed)

    assert_finite(its.t, {
        logger(
            EError,
            "  your intersection produced a non-finite intersection distance");
        logger(EError, "  offending shape: %s", its.instance->shape());
    });
    assert_condition(its.t >= Epsilon, {
        logger(EError,
               "  your intersection is susceptible to self-intersections");
        logger(EError, "  offending shape: %s", its.instance->shape());
        logger(EError,
               "  returned t: %.3g (smaller than Epsilon = %.3g)",
               its.t,
               Epsilon);
    });
}

bool Instance::checkTransparent(const Intersection &its, Sampler &rng) const {
    // this function is used to check if the intersection is transparent 
    // due to alpha masking value. 
    if (!m_alpha) {
        return false;
    }

    const float alpha = m_alpha->scalar(its.uv);
    if (rng.next() > alpha) {
        return true;
    }

    return false;
}

bool Instance::intersect(const Ray &worldRay, Intersection &its,
                         Sampler &rng) const {

    if (!m_transform) {
        // fast path, if no transform is needed
        const Ray localRay        = worldRay;
        const bool wasIntersected = m_shape->intersect(localRay, its, rng);
        if (wasIntersected) {
            if (checkTransparent(its, rng)) {
                return false;
            }
            its.instance = this;
            validateIntersection(its);
        }
        return wasIntersected;
    }

    const Intersection oldIts = its;
    const float previousT = its.t;
    //NOT_IMPLEMENTED
    Ray localRay = m_transform->inverse(worldRay);
    // its.t = previousT * localRay.direction.length();
    const float scale = localRay.direction.length();
    localRay = localRay.normalized();
    its.t = previousT * scale;
    // hints:
    // * transform the ray (do not forget to normalize!)
    // * how does its.t need to change?

    const bool wasIntersected = m_shape->intersect(localRay, its, rng);
    if (wasIntersected) {
        if (checkTransparent(its, rng)) {
            its.t = previousT;
            return false;
            }
        
        if (m_volume){            
            Ray nextlocalray = localRay;
            nextlocalray.origin = localRay(its.t + Epsilon * scale);
            Intersection nextIts = oldIts;
            nextIts.t = Infinity;
            float insideT = its.t;
            float tMin = 0.0f;
        
            if (m_shape->intersect(nextlocalray, nextIts, rng)){
                insideT = nextIts.t + Epsilon * scale; 
                tMin    = its.t;
            }
            float distance = 0.0f;
            if (m_volume->sampleDistance(tMin, insideT, scale, localRay, rng, distance) && distance < oldIts.t){
                its.instance = this;
                its.shadingNormal = squareToUniformSphere(rng.next2D());
                // update tangent according to the new shading normal
                its.tangent = its.shadingFrame().tangent.normalized();
                its.t = distance;
                its.position = worldRay(its.t);
                return true;
            } else {
                its = oldIts;
                return false;
            }

        }

        its.instance = this;
        validateIntersection(its);
        
        its.t /= scale;
        transformFrame(its, -localRay.direction);
        
    } else {
        its.t = previousT;
    }

    return wasIntersected;
}

Bounds Instance::getBoundingBox() const {
    if (!m_transform) {
        // fast path
        return m_shape->getBoundingBox();
    }

    const Bounds untransformedAABB = m_shape->getBoundingBox();
    if (untransformedAABB.isUnbounded()) {
        return Bounds::full();
    }

    Bounds result;
    for (int point = 0; point < 8; point++) {
        Point p = untransformedAABB.min();
        for (int dim = 0; dim < p.Dimension; dim++) {
            if ((point >> dim) & 1) {
                p[dim] = untransformedAABB.max()[dim];
            }
        }
        p = m_transform->apply(p);
        result.extend(p);
    }
    return result;
}

Point Instance::getCentroid() const {
    if (!m_transform) {
        // fast path
        return m_shape->getCentroid();
    }

    return m_transform->apply(m_shape->getCentroid());
}

AreaSample Instance::sampleArea(Sampler &rng) const {
    AreaSample sample = m_shape->sampleArea(rng);
    transformFrame(sample, Vector());
    return sample;
}

AreaSample Instance::improvedSampleArea(const Point &point, Sampler &rng) const{
    Point local_origin = m_transform->inverse(point);
    AreaSample sample = m_shape->improvedSampleArea(local_origin, rng);
    transformFrame(sample, Vector());
    return sample;
}

} // namespace lightwave

REGISTER_CLASS(Instance, "instance", "default")

// ---- back up here ----
// #include <lightwave/core.hpp>
// #include <lightwave/instance.hpp>
// #include <lightwave/registry.hpp>
// #include <lightwave/sampler.hpp>

// namespace lightwave {

// void Instance::transformFrame(SurfaceEvent &surf, const Vector &wo) const {
//     surf.position       = m_transform->apply(surf.position);

//     Vector bitangent    = surf.shadingFrame().bitangent;
//     Vector world_tangent= m_transform->apply(surf.tangent);
//     // compute the real pA which should be 1/(4pi*r^2) for the sphere, r is the 'scale' in xml
//     surf.pdf            /= world_tangent.cross(m_transform->apply(bitangent)).length();
//     surf.tangent        = world_tangent.normalized();
//     surf.geometryNormal = m_transform->applyNormal(surf.geometryNormal).normalized();
//     surf.shadingNormal  = m_transform->applyNormal(surf.shadingNormal).normalized();
    
//     if (m_normal) { // newly added shading normal 
//         Vector normalMap = (2.0f * m_normal->evaluate(surf.uv) - Color(1.0f)).data();
//         // update shading normal of surface event
//         surf.shadingNormal = (surf.shadingFrame().toWorld(normalMap)).normalized();
//         // update tangent according to the new shading normal
//         surf.tangent = surf.shadingFrame().tangent.normalized();
//     }

// }

// inline void validateIntersection(const Intersection &its) {
//     // use the following macros to make debugginer easier:
//     // * assert_condition(condition, { ... });
//     // * assert_normalized(vector, { ... });
//     // * assert_ortoghonal(vec1, vec2, { ... });
//     // * assert_finite(value or vector or color, { ... });

//     // each assert statement takes a block of code to execute when it fails
//     // (useful for printing out variables to narrow done what failed)

//     assert_finite(its.t, {
//         logger(
//             EError,
//             "  your intersection produced a non-finite intersection distance");
//         logger(EError, "  offending shape: %s", its.instance->shape());
//     });
//     assert_condition(its.t >= Epsilon, {
//         logger(EError,
//                "  your intersection is susceptible to self-intersections");
//         logger(EError, "  offending shape: %s", its.instance->shape());
//         logger(EError,
//                "  returned t: %.3g (smaller than Epsilon = %.3g)",
//                its.t,
//                Epsilon);
//     });
// }

// bool Instance::checkTransparent(const Intersection &its, Sampler &rng) const {
//     // this function is used to check if the intersection is transparent 
//     // due to alpha masking value. 
//     if (!m_alpha) {
//         return false;
//     }

//     const float alpha = m_alpha->scalar(its.uv);
//     if (rng.next() > alpha) {
//         return true;
//     }

//     return false;
// }

// bool Instance::intersect(const Ray &worldRay, Intersection &its,
//                          Sampler &rng) const {

//     if (!m_transform) {
//         // fast path, if no transform is needed
//         const Ray localRay        = worldRay;
//         const bool wasIntersected = m_shape->intersect(localRay, its, rng);
//         if (wasIntersected) {
//             if (checkTransparent(its, rng)) {
//                 return false;
//             }
//             its.instance = this;
//             validateIntersection(its);
//         }
//         return wasIntersected;
//     }

//     const float previousT = its.t;
//     //NOT_IMPLEMENTED
//     Ray localRay = m_transform->inverse(worldRay);
//     // its.t = previousT * localRay.direction.length();
//     const float scale = localRay.direction.length();
//     localRay = localRay.normalized();
//     its.t = previousT * scale;
//     // hints:
//     // * transform the ray (do not forget to normalize!)
//     // * how does its.t need to change?

//     const bool wasIntersected = m_shape->intersect(localRay, its, rng);
//     if (wasIntersected) {
//         if (checkTransparent(its, rng)) {
//             its.t = previousT;
//             return false;
//             }
//         its.instance = this;
//         validateIntersection(its);
//         // hint: how does its.t need to change?
//         // Ray world_localRay = m_transform->apply(localRay);
//         // its.t = its.t * world_localRay.direction.length();
//         its.t /= scale;

//         // if (its.frame.normal.dot(localRay.direction) > 0) {
//         //     /// TODO: hack, just for testing
//         //     its.frame.tangent *= -1;
//         //     its.frame.bitangent *= -1;
//         //     its.frame.normal *= -1;

//         //     its.geoFrame.tangent *= -1;
//         //     its.geoFrame.bitangent *= -1;
//         //     its.geoFrame.normal *= -1;
//         // }
//         transformFrame(its, -localRay.direction);
//         // if (its.frame.normal.dot(worldRay.direction) > 0) {
//         //     /// TODO: hack, just for testing
//         //     its.frame.tangent *= -1;
//         //     its.frame.bitangent *= -1;
//         //     its.frame.normal *= -1;

//         //     its.geoFrame.tangent *= -1;
//         //     its.geoFrame.bitangent *= -1;
//         //     its.geoFrame.normal *= -1;
//         // }
//     } else {
//         its.t = previousT;
//     }

//     return wasIntersected;
// }

// Bounds Instance::getBoundingBox() const {
//     if (!m_transform) {
//         // fast path
//         return m_shape->getBoundingBox();
//     }

//     const Bounds untransformedAABB = m_shape->getBoundingBox();
//     if (untransformedAABB.isUnbounded()) {
//         return Bounds::full();
//     }

//     Bounds result;
//     for (int point = 0; point < 8; point++) {
//         Point p = untransformedAABB.min();
//         for (int dim = 0; dim < p.Dimension; dim++) {
//             if ((point >> dim) & 1) {
//                 p[dim] = untransformedAABB.max()[dim];
//             }
//         }
//         p = m_transform->apply(p);
//         result.extend(p);
//     }
//     return result;
// }

// Point Instance::getCentroid() const {
//     if (!m_transform) {
//         // fast path
//         return m_shape->getCentroid();
//     }

//     return m_transform->apply(m_shape->getCentroid());
// }

// AreaSample Instance::sampleArea(Sampler &rng) const {
//     AreaSample sample = m_shape->sampleArea(rng);
//     transformFrame(sample, Vector());
//     return sample;
// }

// AreaSample Instance::improvedSampleArea(const Point &point, Sampler &rng) const{
//     Point local_origin = m_transform->inverse(point);
//     AreaSample sample = m_shape->improvedSampleArea(local_origin, rng);
//     transformFrame(sample, Vector());
//     return sample;
// }

// } // namespace lightwave

// REGISTER_CLASS(Instance, "instance", "default")