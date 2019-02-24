#include <benchmark/benchmark.h>

#include <array>
#include <vector>
#include <random>

#include <immintrin.h>

using T = unsigned int;

// some infra-structure to automatically generate benchmarks:
template <size_t D, typename Impl>
static void do_benchmark(benchmark::State& state, Impl impl) {
    const auto targetLevel = static_cast<unsigned>(state.range(0));

    std::mt19937_64 prng;
    std::uniform_int_distribution<T> distr;

    std::vector<std::array<T, D> > points(1000);
    for(auto& pt : points)
        std::generate(pt.begin(), pt.end(), [&] {return distr(prng);});

    const auto* it = points.data();
    const auto* end = it + points.size();

    for (auto _ : state) {
        auto res = impl(*it, targetLevel);
        benchmark::DoNotOptimize(res);
        if (++it == end) it = points.data();
    }
}

#define ADD_IMPL_BENCHMARK(X) \
 template <size_t D> \
 static void BM_ ## X (benchmark::State& state) {do_benchmark<D>(state, impl_ ## X<D>);} \
 BENCHMARK_TEMPLATE(BM_ ## X, 1)->RangeMultiplier(2)->Range(1, 32); \
 BENCHMARK_TEMPLATE(BM_ ## X, 2)->RangeMultiplier(2)->Range(1, 16); \
 BENCHMARK_TEMPLATE(BM_ ## X, 3)->RangeMultiplier(2)->Range(1, 10); \
 BENCHMARK_TEMPLATE(BM_ ## X, 4)->RangeMultiplier(2)->Range(1,  8); \
 BENCHMARK_TEMPLATE(BM_ ## X, 5)->RangeMultiplier(2)->Range(1,  6);

////////////////////////////////
// CONTENDERS

// naive implementation
template <size_t D>
constexpr static auto impl_naive(const std::array<T, D>& coords, unsigned targetLevel) -> typename std::enable_if<(D > 1), T>::type {
    unsigned int result = 0u;
    unsigned int bit = 0;

    for(auto l = 0u; l != targetLevel; l++) {
        for(auto d = 0u; d != D; d++) {
            result |= ((coords[d] >> l) & 1) << bit++;
        }
    }

    return result;
}

template <size_t D>
constexpr static auto impl_naive(const std::array<T, D>& coords, unsigned) -> typename std::enable_if<D == 1, T>::type  {
    return coords[0];
}

// https://graphics.stanford.edu/~seander/bithacks.html#InterleaveBMN
unsigned int clever_interleave2(unsigned int x, unsigned int y) {
    static const unsigned int B[] = {0x55555555, 0x33333333, 0x0F0F0F0F, 0x00FF00FF};
    static const unsigned int S[] = {1, 2, 4, 8};

    x = (x | (x << S[3])) & B[3];
    x = (x | (x << S[2])) & B[2];
    x = (x | (x << S[1])) & B[1];
    x = (x | (x << S[0])) & B[0];

    y = (y | (y << S[3])) & B[3];
    y = (y | (y << S[2])) & B[2];
    y = (y | (y << S[1])) & B[1];
    y = (y | (y << S[0])) & B[0];

    return x | (y << 1);
}

uint32_t clever_interleave2_64(unsigned int x, unsigned int y) {
    static constexpr uint64_t B[] = {0x55555555'55555555,
                                     0x33333333'33333333,
                                     0x0F0F0F0F'0F0F0F0F,
                                     0x00FF00FF'00FF00FF};
    static constexpr unsigned int S[] = {1, 2, 4, 8};

    uint64_t z = x | (static_cast<uint64_t>(y) << 32);

    z = (z | (z << S[3])) & B[3];
    z = (z | (z << S[2])) & B[2];
    z = (z | (z << S[1])) & B[1];
    z = (z | (z << S[0])) & B[0];

    z = z | (z >> 31);

    return static_cast<uint32_t>(z);
}



template <size_t D>
constexpr static auto impl_clever(const std::array<T, D>& coords, unsigned) -> typename std::enable_if<D == 1, T>::type  {
    return coords[0];
}

template <size_t D>
constexpr static auto impl_clever(const std::array<T, D>& coords, unsigned) -> typename std::enable_if<D == 2, T>::type  {
    constexpr uint64_t B[] = {0x55555555'55555555,
                              0x33333333'33333333,
                              0x0F0F0F0F'0F0F0F0F,
                              0x00FF00FF'00FF00FF};
    constexpr unsigned int S[] = {1, 2, 4, 8};

    uint64_t z = coords[0] | (static_cast<uint64_t>(coords[1]) << 32);

    z = (z | (z << S[3])) & B[3];
    z = (z | (z << S[2])) & B[2];
    z = (z | (z << S[1])) & B[1];
    z = (z | (z << S[0])) & B[0];

    z = z | (z >> 31);

    return static_cast<uint32_t>(z);
}

template <size_t D>
constexpr static auto impl_clever(const std::array<T, D>& coords, unsigned) -> typename std::enable_if<D == 4, T>::type  {
    constexpr uint64_t B[] = {0x55555555'55555555,
                              0x33333333'33333333,
                              0x0F0F0F0F'0F0F0F0F};
    constexpr unsigned int S[] = {1, 2, 4};

    uint64_t z = 0;
    for(int i=0; i != 4; i++)
        z |= static_cast<uint64_t>(coords[i] & 0xff) << (16 * i);

    z = (z | (z << S[2])) & B[2];
    z = (z | (z << S[1])) & B[1];
    z = (z | (z << S[0])) & B[0];

    z = z | ((z & 0xffff0000ffff0000) >> 15);
    z = z | (z >> 31);

    return static_cast<uint32_t>(z);
}


template <size_t D>
constexpr static auto impl_clever(const std::array<T, D>& coords, unsigned targetLevel) -> typename std::enable_if<(D == 3 || D == 5), T>::type  {
    return impl_naive(coords, targetLevel);
}



// PDEP based implementation
// compute BitMask in which every D-th bit starting with the I-th is 1
template <typename T, size_t D, size_t I, bool = false>
struct impl_pdep_bitmask;

template <typename T, size_t D, size_t I>
struct impl_pdep_bitmask<T, D, I, true> {
    static constexpr T mask = 0;
};

template <typename T, size_t D, size_t I>
struct impl_pdep_bitmask<T, D, I, false> {
    static constexpr T mask = (T{1} << I) | impl_pdep_bitmask<T, I+D, D, I+D >= std::numeric_limits<T>::digits>::mask;
};

// call pdep D times
template <size_t D, size_t I = 0, bool = false>
struct impl_pdep_invoke;

template <size_t D, size_t I>
struct impl_pdep_invoke<D, I, true> {
    T operator() (const std::array<T, D>& coords) {return 0;}
};

template <size_t D, size_t I>
struct impl_pdep_invoke<D, I, false> {
    constexpr auto operator() (const std::array<T, D>& coords) noexcept {
        if constexpr (D == I + 1) {
            impl_pdep_invoke<D, I + 1, D == I + 1> rec;
            return _pdep_u64(coords[I], impl_pdep_bitmask<T, D, I>::mask) | rec(coords);
        } else {
            constexpr auto mask = impl_pdep_bitmask<uint32_t, D, I>::mask | (static_cast<uint64_t>(impl_pdep_bitmask<uint32_t, D, I+1>::mask) << 32);
            auto values = coords[I] | (static_cast<uint64_t>(coords[I+1]) << 32);
            auto deposited = _pdep_u64(values, mask);

            impl_pdep_invoke<D, I + 2, D < I + 2> rec;
            return static_cast<uint32_t>(deposited | deposited >> 32) | rec(coords);
        }
    }
};

template <size_t D>
constexpr static T impl_pdep(const std::array<T, D>& coords, unsigned) noexcept {
    unsigned int result = 0u;
    impl_pdep_invoke<D> rec;
    return rec(coords);
}

ADD_IMPL_BENCHMARK(clever)
ADD_IMPL_BENCHMARK(naive)
ADD_IMPL_BENCHMARK(pdep)
