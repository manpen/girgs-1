#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <limits>

namespace girgs {

/// Produces a mask with X=consecutive_ones starting at the index-th LSB.
/// The pattern is repeated every 2X bits.
constexpr uint64_t bitInterleavingMask(size_t consecutive_ones, size_t index = 0u) {
    return index >= std::numeric_limits<uint64_t>::digits
           ? 0ull
           : (((1ull << consecutive_ones) - 1) << index) | bitInterleavingMask(consecutive_ones, index + 2*consecutive_ones);
}

/// Helper to transform cell coordinates to cell rank.
template <size_t D>
struct BitInterleavingHelper {
    // Here we define a generic "fallback" solution and have more optimized specialization down below.
    // Observe that we do not rely on the PDEP instruction for i) portability and ii) as it has a latency
    // of ~18 cycles on modern AMD processors.

    /**
     * Interleave the bits of the coordinates, let X[i,j] be the i-th bit (counting from LSB) of coordinate j,
     * and let D' = D-1, and t = targetLevel-1. Then return
     *  X[t, D'] o X[t, D'-1] o ... o X[t, 0]   o  X[t-1, D'] o ... o X[t-1, 0]   o  ...  o  X[0, D'] o ... o X[0, 0]
     *
     * The following in a naive implementation of this bit interleaving:
     */
#if __cplusplus >= 201402L
    constexpr
#endif
    static uint32_t interleave(const std::array<uint32_t, D>& coords, unsigned targetLevel) noexcept {
        assert(targetLevel < std::numeric_limits<uint32_t>::digits / D);

        unsigned int result = 0u;

        for(auto l = 0u; l != targetLevel; l++) {
            for(auto d = 0u; d != D; d++) {
                result |= ((coords[d] >> l) & 1) << (D*l + d);
            }
        }

        return result;
    }
};

template <>
struct BitInterleavingHelper<1> {
    constexpr static uint32_t interleave(const std::array<uint32_t, 1>& coords, unsigned) noexcept {
        return coords[0];
    }
};

template <>
struct BitInterleavingHelper<2> {
#if __cplusplus >= 201402L
    constexpr
#endif
    static uint32_t interleave(const std::array<uint32_t, 2>& coords, unsigned) noexcept {
        uint64_t z = coords[0] | (static_cast<uint64_t>(coords[1]) << 32);

        z = (z | (z << 8)) & bitInterleavingMask(8);
        z = (z | (z << 4)) & bitInterleavingMask(4);
        z = (z | (z << 2)) & bitInterleavingMask(2);
        z = (z | (z << 1)) & bitInterleavingMask(1);

        z = z | (z >> 31);

        return static_cast<uint32_t>(z);
    }
};

template <>
struct BitInterleavingHelper<4> {
#if __cplusplus >= 201402L
    constexpr
#endif
    static uint32_t interleave(const std::array<uint32_t, 4>& coords, unsigned) noexcept {
        uint64_t z = 0;
        for(int i=0; i != 4; i++)
            z |= static_cast<uint64_t>(coords[i] & 0xff) << (16 * i);

        z = (z | (z << 4)) & bitInterleavingMask(4);
        z = (z | (z << 2)) & bitInterleavingMask(2);
        z = (z | (z << 1)) & bitInterleavingMask(1);

        z = z | ((z & bitInterleavingMask(16)) >> 15);
        z = z | (z >> 31);

        return static_cast<uint32_t>(z);
    }
};

// Compile-Time Unit-Tests
#if (__cplusplus >= 201402L)
#include <girgs/BitInterleavingHelperCXX14.inl>
#endif

} // namespace