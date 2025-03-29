/**
 * @file hash.hpp
 * @brief Includes hashing functions used by data structures or to seed random
 * number generators.
 */

#pragma once

#include <lightwave/core.hpp>

namespace lightwave::hash {

/// @brief The FNV-1a (Fowler–Noll–Vo) hash function with 64-bit state.
class fnv1a {
    /// @brief The current state of the hash function.
    uint64_t hash = 0xCBF29CE484222325;

public:
    /// @brief Constructs a FNV-1a hash for a given sequence of integers.
    template <typename... Ts> fnv1a(Ts... args) { (operator<<(args), ...); }

    /// @brief Updates the state by hashing a given integer.
    template <typename T> fnv1a &operator<<(T data) {
        for (size_t pos = 0; pos < sizeof(T); pos++) {
            uint8_t byte = data >> (8 * pos);
            hash         = (hash ^ byte) * 0x100000001b3;
        }
        return *this;
    }

    /// @brief Returns the current state of the hash function.
    operator uint64_t() { return hash; }
};

} // namespace lightwave::hash
