#include "lightwave/registry.hpp"
#include "lightwave/sampler.hpp"

#include "pcg32.h"
#include "primes.h"

namespace lightwave {

class Halton final : public Sampler {
    uint64_t m_seed;
    int m_sample_index;

    static constexpr size_t m_prime_table_size = 1024;
    std::array<uint32_t, m_prime_table_size> m_primes = {PRIMES};

    inline float radical_inverse(uint32_t dimension, uint32_t sample_index) {
        uint32_t limit = ~0u / dimension - dimension;
        float invBase = 1.f / dimension, invBaseM = 1.f;
        uint32_t reversedDigits = 0;
        while (sample_index && reversedDigits < limit) {
            uint32_t next = sample_index / dimension;
            uint32_t digit = sample_index - next * dimension;
            reversedDigits = reversedDigits * dimension + digit;
            invBaseM *= invBase;
            sample_index = next;
        }
        return std::min(reversedDigits * invBaseM, 9.999999e-1f);
    }

public:
    Halton(const Properties &properties) : Sampler(properties) { m_seed = 0; }

    void seed(int sampleIndex) override {
        m_seed = 0;
        m_sample_index = sampleIndex;
    }

    void seed(const Point2i &pixel, int sampleIndex) override {
        const uint64_t a = (uint64_t(pixel.x()) << 32) ^ pixel.y();
        pcg32 pcg(1337, a);
        seed(pcg.nextUInt() ^ sampleIndex);
    }

    float next() override {
        m_seed = m_seed % m_prime_table_size;
        return radical_inverse(m_primes[m_seed++], m_sample_index);
    }

    Point2 next2D() override { return {next(), next()}; }

    ref<Sampler> clone() const override {
        return std::make_shared<Halton>(*this);
    }

    std::string toString() const override {
        return tfm::format(
            "Halton[\n"
            "  count = %d\n"
            "]",
            m_samplesPerPixel);
    }
};

}

REGISTER_SAMPLER(Halton, "halton")

// #include <lightwave.hpp>

// #include "pcg32.h"
// #include "primes.h"

// namespace lightwave {
//     class Halton : public Sampler {
//         uint64_t m_seed;
//         pcg32 m_pcg;
//         PrimeGenerator m_primeGenerator;

//         int m_base;
//         int m_index;
//         float m_offset;

//     public:
//         Halton(const Properties &properties): Sampler(properties) {
//             m_seed = properties.get<int>("seed", 1337);
//         }

//         void seed(int sampleIndex) override {}
    
//         void seed(const Point2i &pixel, int sampleIndex) override {    
//             m_primeGenerator.resetIdx();
//             m_base = m_primeGenerator.getNextPrime();

//             const uint64_t a = (uint64_t(pixel.x()) << 32) ^ pixel.y();
//             m_pcg.seed(m_seed, a);
//             m_offset = m_pcg.nextFloat();

//             m_index = sampleIndex;
//         }

//         float next() override {
//             float result = halton(m_base, m_index) + m_offset;
//             if (result >= 1.0f) result -= 1.0f;
//             m_base = m_primeGenerator.getNextPrime();
//             return result; 
//         }

//         float halton(int base, int index) {
//             float invBase = (float)1 / (float)base, invBaseM = 1;
//             uint64_t reversedDigits = 0;

//             while (index) {
//                 uint64_t next = index / base;
//                 uint64_t digit = index - next * base;
//                 reversedDigits = reversedDigits * base + digit;
//                 invBaseM *= invBase;
//                 index = next;
//             }
//             return min(reversedDigits * invBaseM, 1 - Epsilon);
//         }

//         ref<Sampler> clone() const override {
//             return std::make_shared<Halton>(*this);
//         }

//         std::string toString() const override {
//             return tfm::format(
//                 "Halton[\n"
//                 "  count = %d\n"
//                 "]",
//                 m_samplesPerPixel
//             );
//         }
//     };
// }

// REGISTER_SAMPLER(Halton, "halton")
