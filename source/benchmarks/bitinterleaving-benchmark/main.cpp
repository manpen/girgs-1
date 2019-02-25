#include <benchmark/benchmark.h>

#include <array>
#include <vector>
#include <random>

#ifdef USE_PDEP
#include <immintrin.h>
#endif

//#define CHECK_XREF_ONLY

using T = unsigned int;

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

template<unsigned int D>
constexpr static uint32_t impl_pattern(const std::array<uint32_t,D>& coords, unsigned) {

    uint32_t res = 0u;
    for(int i=0; i<32; ++i){
        res |= ((coords[i%D]>>i/D) & 1) << i; // res[i] = coords[i%D][i/D]
    }

    return res;
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


#ifdef USE_PDEP
constexpr uint32_t highbits(int x) {
    return static_cast<uint32_t>((1llu << x) - 1);
}

static uint32_t impl_pdep(uint32_t a) noexcept {
    return a;
}

static uint32_t impl_pdep(uint32_t a, uint32_t b) noexcept {
    auto values = (b << 16) | (a & 0xffff);
    auto deposited = _pdep_u64(values, 0x55555555'55555555ull);
    return static_cast<uint32_t>(deposited | (deposited >> 31));
}

static uint32_t impl_pdep(uint32_t a, uint32_t b, uint32_t c) noexcept {
    const auto rec = impl_pdep(a, b);

    const auto values = ((c & highbits(10)) << 22) | (rec & highbits(22));
    const auto deposited = _pdep_u64(values, 0b00'100100100100100100100100100100'11011011011011011011011011011011);

    return (deposited & highbits(32)) | (deposited >> 32);
}

static uint32_t impl_pdep(uint32_t a, uint32_t b, uint32_t c, uint32_t d) noexcept {
    return impl_pdep(impl_pdep(a, c), impl_pdep(b, d));
}

static uint32_t impl_pdep(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e) noexcept {
    const auto rec = impl_pdep(a,b,c,d);

    const auto values = ((e & highbits(6)) << 26) | (rec & highbits(26));
    const auto deposited = _pdep_u64(values, 0b00'100001000010000100001000010000'11011110111101111011110111101111);

    return (deposited & highbits(32)) | (deposited >> 32);
}

// array based
template <size_t D>
static auto impl_pdep(const std::array<uint32_t, D>& c, unsigned = 0) noexcept -> typename std::enable_if<D == 1, uint32_t>::type {return impl_pdep(c[0]                        );}

template <size_t D>
static auto impl_pdep(const std::array<uint32_t, D>& c, unsigned = 0) noexcept -> typename std::enable_if<D == 2, uint32_t>::type {return impl_pdep(c[0], c[1]                  );}

template <size_t D>
static auto impl_pdep(const std::array<uint32_t, D>& c, unsigned = 0) noexcept -> typename std::enable_if<D == 3, uint32_t>::type {return impl_pdep(c[0], c[1], c[2]            );}

template <size_t D>
static auto impl_pdep(const std::array<uint32_t, D>& c, unsigned = 0) noexcept -> typename std::enable_if<D == 4, uint32_t>::type {return impl_pdep(c[0], c[1], c[2], c[3]      );}

template <size_t D>
static auto impl_pdep(const std::array<uint32_t, D>& c, unsigned = 0) noexcept -> typename std::enable_if<D == 5, uint32_t>::type {return impl_pdep(c[0], c[1], c[2], c[3], c[4]);}
#endif

////////////////////////////////
// BENCHMARK

// some infra-structure to automatically generate benchmarks:
template <size_t D, typename Impl>
static void do_benchmark(benchmark::State& state, Impl impl) {
    const auto targetLevel = static_cast<unsigned>(state.range(0));

    std::mt19937_64 prng(0);
    std::uniform_int_distribution<T> distr;

    std::vector< std::array<T, D> > points(123*D + 234*targetLevel);

    int i = 0;
    for(auto& pt : points) {
        if (i < D) {
            for(int j = 0; j < D; j++)
                pt[j] = -1 * (i == j);
        } else {
            std::generate(pt.begin(), pt.end(), [&] { return distr(prng); });
        }

        // cross-reference with naive implementation
        {
            const auto sig_mask = static_cast<uint32_t>((1llu << targetLevel * D) - 1);
            auto res = impl(pt, targetLevel);
            const auto ref = impl_naive<D>(pt, targetLevel) & sig_mask;
            const auto test = res & sig_mask;
            assert(ref == test);
        }

        i++;
    }

    const auto* it = points.data();
    const auto* end = it + points.size();

    for (auto _ : state) {
#ifdef CHECK_XREF_ONLY
    state.SkipWithError("Measurements skipped due to CHECK_XREF_ONLY");
#endif

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


ADD_IMPL_BENCHMARK(naive)
ADD_IMPL_BENCHMARK(clever)
ADD_IMPL_BENCHMARK(pattern)

#ifdef USE_PDEP
    ADD_IMPL_BENCHMARK(pdep)
#endif
