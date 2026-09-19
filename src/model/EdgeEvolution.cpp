#include "od/model/EdgeEvolution.hpp"

namespace od::model {

double linearProbability(double p0, double k, int t) {
    double p = p0 + k * static_cast<double>(t);

    if (p > 1.0) return 1.0;
    if (p < 0.0) return 0.0;
    return p;
}

void evolveEdges(Graph& graph, double currentP, std::mt19937& rng) {
    int n = graph.getNumVertices();
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    for (int u = 0; u < n; ++u) {
        for (int v = u + 1; v < n; ++v) {
            if (graph.hasEdge(u, v)) {
                continue;
            }
            if (dist(rng) < currentP) {
                graph.addEdge(u, v, dist(rng));
            }
        }
    }
}

} // namespace od::model