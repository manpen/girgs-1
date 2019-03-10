#include <iostream>
#include <sstream>

#ifdef __unix__
#include <unistd.h> // gethostname
#endif

#include <algorithm>
#include <cassert>

#include <hypergirgs/HyperbolicTree.h>
#include <hypergirgs/Hyperbolic.h>
#include <hypergirgs/ScopedTimer.h>

#include "CounterPerThread.h"


double benchmark(std::ostream& os, const std::string& host, int iter, unsigned int n, unsigned int avgDeg, double alpha, double T, unsigned int seed = 0) {
    CounterPerThread<uint64_t> counter_num_edges;

    auto R = hypergirgs::calculateNkGenRadius(n, alpha, T, avgDeg);
    std::cout << "TargetRadius: " << R << "\n";

    double time_total, time_points, time_preprocess, time_sample;
    {
        ScopedTimer tot_timer("Total", time_total);

        // Sample points
        std::vector<double> radii, angles;
        {
            ScopedTimer timer("Generate points", time_points);
            radii = hypergirgs::sampleRadii(n, alpha, R, seed++);
            angles = hypergirgs::sampleAngles(n, seed++);
        }

        // Preprocess
        auto addEdge = [&counter_num_edges] (int, int, int tid) {counter_num_edges.add(tid);};
        hypergirgs::HyperbolicTree<decltype(addEdge)> generator = [&] {
            ScopedTimer timer("Preprocess", time_preprocess);
            return hypergirgs::makeHyperbolicTree(radii, angles, T, R, addEdge, true);
        }();

        // Generate edges
        {
            ScopedTimer timer("Generate edges", time_sample);
            generator.generate(seed);
        }
    }

    const auto num_edges = counter_num_edges.total();
    std::cout << "Generated " << num_edges << " edges (avg-deg: " << (2.0 * num_edges / n) << ")\n";

    // Logging
    {
        std::stringstream ss;

        ss << "[CSV] "
           << host
           << "," << "hypergirg"
           << "," << iter
           << "," << T
           << "," << alpha
           << "," << n
           << "," << avgDeg
           << "," << R
           << "," << time_total
           << "," << num_edges
           << "," << time_points
           << "," << time_preprocess
           << "," << time_total;

        os << ss.str() << std::endl;
    }

    return time_total;
}

int main(int argc, char* argv[]) {
    unsigned int seed = 0;
    const unsigned n0 = 1e4;
    const unsigned nMax = 1e6;
    const unsigned steps_per_dec = 3;
    const double timeout = 100 * 1e3; // ms

    // obtain hostname
    const auto host = [] {
#ifdef __unix__
        char hostname[128];
        if (gethostname(hostname, 128)) {
            return std::string("n/a");
        }
        return std::string(hostname);
#else
        return "n/a";
#endif
    }();

    if (argc > 1)
        seed = atoi(argv[1]);

    // Print Header
    {
        std::stringstream ss;

        ss << "[CSV] "
           "host,algo,iter,T,alpha,n,deg,R,time,edges,samplingTime,preprocessTime,totalTime";

        std::cout << ss.str() << std::endl;
    }

    for(unsigned int iter = 0; iter != 5; ++iter) {
        for(const double T : {0.0, 0.5, 2.0, 5.0, 10.0}) {
            if (T > 0) continue;
        for (const double ple : {3.0}) {
        for (const int avgDeg : {10, 100}) {
            const auto alpha = (ple - 1.0) / 2.0;
            int ni = 0;
            bool skip = false;

            for(auto n = n0; n <= nMax; n = n0 * std::pow(10.0, 1.0 * ni / steps_per_dec), ++ni) {
                std::clog << "\033[1miter=" << iter << ", T=" << T << ", PLE=" << ple << ", n=" << n << ", avgDeg=" << avgDeg << "\033[21m\n";

                double time;
                if (!skip) {
                    // if last (smaller) problem took too long, we skip this one
                    // we do not use break to make sure seed stays consistent
                    time = benchmark(std::cout, host, iter, n, avgDeg, alpha, T, seed);
                    skip |= time > timeout;
                }

                seed += 10;
            }
        }}}
    }

    return 0;
}
