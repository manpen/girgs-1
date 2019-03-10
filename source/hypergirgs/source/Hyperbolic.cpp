
#include <hypergirgs/Hyperbolic.h>
#include <hypergirgs/HyperbolicTree.h>

#include <random>
#include <fstream>
#include <cmath>
#include <mutex>

#include <omp.h>


namespace hypergirgs {


static double getExpectedDegree(double n, double alpha, double R) {
    double gamma = 2*alpha+1;
    double xi = (gamma-1)/(gamma-2);
    double firstSumTerm = exp(-R/2);
    double secondSumTerm = exp(-alpha*R)*(alpha*(R/2)*((PI/4)*pow((1/alpha),2)-(PI-1)*(1/alpha)+(PI-2))-1);
    double expectedDegree = (2/PI)*xi*xi*n*(firstSumTerm + secondSumTerm);
    return expectedDegree;
}

static double searchTargetRadiusForColdGraphs(double n, double k, double alpha, double epsilon) {
    double gamma = 2*alpha+1;
    double xiInv = ((gamma-2)/(gamma-1));
    double v = k * (PI/2)*xiInv*xiInv;
    double currentR = 2*log(n / v);
    double lowerBound = currentR/2;
    double upperBound = currentR*2;
    assert(getExpectedDegree(n, alpha, lowerBound) > k);
    assert(getExpectedDegree(n, alpha, upperBound) < k);
    do {
        currentR = (lowerBound + upperBound)/2;
        double currentK = getExpectedDegree(n, alpha, currentR);
        if (currentK < k) {
            upperBound = currentR;
        } else {
            lowerBound = currentR;
        }
    } while (abs(getExpectedDegree(n, alpha, currentR) - k) > epsilon );
    return currentR;
}

static double getTargetRadius(double n, double m, double alpha=1, double T=0, double epsilon = 0.01) {
    double result;
    double plexp = 2*alpha+1;
    double targetAvgDegree = (m/n)*2;
    double xiInv = ((plexp-2)/(plexp-1));
    if (T == 0) {
        double v = targetAvgDegree * (PI/2)*xiInv*xiInv;
        result = 2*log(n / v);
        result = searchTargetRadiusForColdGraphs(n, targetAvgDegree, alpha, epsilon);
    } else {
        double beta = 1/T;
        if (T < 1){//cold regime
            double Iinv = ((beta/PI)*sin(PI/beta));
            double v = (targetAvgDegree*Iinv)*(PI/2)*xiInv*xiInv;
            result = 2*log(n / v);
        } else {//hot regime
            double v = targetAvgDegree*(1-beta)*pow((PI/2), beta)*xiInv*xiInv;
            result = 2*log(n/v)/beta;
        }
    }
    return result;
}

double calculateNkGenRadius(int n, double alpha, double T, int deg) {
    return getTargetRadius(n, 0.5 * n * deg, alpha, T);
}



double calculateRadius(int n, double alpha, double T, int deg) {
    return 2 * log(n * 2 * alpha * alpha * (T == 0 ? 1 / PI : T / sin(PI * T)) /
                   (deg * (alpha - 0.5) * (alpha - 0.5)));
}




double hyperbolicDistance(double r1, double phi1, double r2, double phi2) {
    return acosh(std::max(1., cosh(r1 - r2) + (1. - cos(phi1 - phi2)) * sinh(r1) * sinh(r2)));
}

std::vector<double> sampleRadii(int n, double alpha, double R, int seed, bool parallel) {
    std::vector<double> result(n);

    auto threads = parallel ? omp_get_max_threads() : 1;
    auto gens = std::vector<hypergirgs::default_random_engine>(threads);
    auto dists = std::vector<std::uniform_real_distribution<>>(threads);
    for (int thread = 0; thread < threads; thread++) {
        gens[thread].seed(seed >= 0 ? seed + thread : std::random_device()());
    }

    const auto invalpha = 1.0 / alpha;
    const auto factor = std::cosh(alpha * R) - 1.0;

    #pragma omp parallel num_threads(threads)
    {
        auto& gen = gens[omp_get_thread_num()];
        auto& dist = dists[omp_get_thread_num()];
        #pragma omp for schedule(static)
        for (int i = 0; i < n; ++i) {
            auto p = dist(gen);
            while (p == 0) p = dist(gen);
            result[i] = acosh(p * factor + 1.0) * invalpha;
        }
    }

    return result;
}

std::vector<double> sampleAngles(int n, int seed, bool parallel) {
    std::vector<double> result(n);
    
    auto threads = parallel ? omp_get_max_threads() : 1;
    auto gens = std::vector<hypergirgs::default_random_engine>(threads);
    auto dists = std::vector<std::uniform_real_distribution<>>();
    for (int thread = 0; thread < threads; thread++) {
        gens[thread].seed(seed >= 0 ? seed + thread : std::random_device()());
        dists.emplace_back(0.0, std::nextafter(2 * PI, 0.0));
    }

    #pragma omp parallel num_threads(threads)
    {
        auto& gen = gens[omp_get_thread_num()];
        auto& dist = dists[omp_get_thread_num()];
        #pragma omp for schedule(static)
        for (int i = 0; i < n; ++i)
            result[i] = dist(gen);
    }

    return result;
}

std::vector<std::pair<int, int> > generateEdges(std::vector<double>& radii, std::vector<double>& angles, double T, double R, int seed) {

    using edge_vector = std::vector<std::pair<int, int>>;
    edge_vector result;

    std::vector<std::pair<
            edge_vector,
            uint64_t[31] /* avoid false sharing */
    > > local_edges(omp_get_max_threads());

    constexpr auto block_size = size_t{1} << 20;

    for(auto& v : local_edges)
        v.first.reserve(block_size);

    std::mutex m;
    auto flush = [&] (const edge_vector& local) {
        std::lock_guard<std::mutex> lock(m);
        result.insert(result.end(), local.cbegin(), local.cend());
    };

    auto addEdge = [&](int u, int v, int tid) {
        auto& local = local_edges[tid].first;
        local.emplace_back(u,v);
        if (local.size() == block_size) {
            flush(local);
            local.clear();
            local.reserve(block_size); // just in case
        }
    };

    auto generator = hypergirgs::makeHyperbolicTree(radii, angles, T, R, addEdge);
    generator.generate(seed);

    for(const auto& v : local_edges)
        flush(v.first);

    return result;
}

} // namespace hypergirgs
