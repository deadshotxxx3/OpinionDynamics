#include "od/model/EdgeEvolution.hpp"

namespace od::model {

namespace {

void tryAddEdge(Graph& graph, int u, int v, double probability,
                std::uniform_real_distribution<double>& dist, std::mt19937& rng,
                int step, std::vector<EdgeEvent>& events) {
    if (dist(rng) < probability) {
        double weight = dist(rng);
        graph.addEdge(u, v, weight);
        events.push_back(EdgeEvent{step, true, u, v, weight});
    }
}

void tryRemoveEdge(Graph& graph, int u, int v, double probability,
                   std::uniform_real_distribution<double>& dist, std::mt19937& rng,
                   int step, std::vector<EdgeEvent>& events) {
    if (dist(rng) < probability) {
        double oldWeight = graph.getNeighbors(u).at(v);
        graph.removeEdge(u, v);
        events.push_back(EdgeEvent{step, false, u, v, oldWeight});
    }
}

} // namespace

void evolveEdges(
    Graph& graph,
    double addProbability,
    double removeProbability,
    std::mt19937& rng,
    int step,
    std::vector<EdgeEvent>& events
) {
    if (addProbability <= 0.0 && removeProbability <= 0.0) {
        return;
    }

    int n = graph.getNumVertices();
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    for (int u = 0; u < n; ++u) {
        for (int v = u + 1; v < n; ++v) {
            bool exists = graph.hasEdge(u, v);

            if (!exists && addProbability > 0.0) {
                tryAddEdge(graph, u, v, addProbability, dist, rng, step, events);
            } else if (exists && removeProbability > 0.0) {
                tryRemoveEdge(graph, u, v, removeProbability, dist, rng, step, events);
            }
        }
    }
}

} // namespace od::model