#include "od/model/EdgeEvolution.hpp"

namespace od::model {

double linearProbability(double p0, double k, int t) {
    double p = p0 + k * static_cast<double>(t);

    if (p > 1.0) return 1.0;
    if (p < 0.0) return 0.0;
    return p;
}

void evolveEdges(
    Graph& graph,
    double addProbability,
    double removeProbability,
    std::mt19937& rng,
    int step,
    std::vector<EdgeEvent>& events
) {
    int n = graph.getNumVertices();
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    for (int u = 0; u < n; ++u) {
        for (int v = u + 1; v < n; ++v) {
            bool exists = graph.hasEdge(u, v);

            if (!exists && addProbability > 0.0) {
                if (dist(rng) < addProbability) {
                    double weight = dist(rng);
                    graph.addEdge(u, v, weight);
                    events.push_back(EdgeEvent{step, true, u, v, weight});
                }
            } else if (exists && removeProbability > 0.0) {
                if (dist(rng) < removeProbability) {
                    const auto& neighbors = graph.getNeighbors(u);
                    auto it = neighbors.find(v);
                    double oldWeight = (it != neighbors.end()) ? it->second : 0.0;
                    graph.removeEdge(u, v);
                    events.push_back(EdgeEvent{step, false, u, v, oldWeight});
                }
            }
        }
    }
}

} // namespace od::model