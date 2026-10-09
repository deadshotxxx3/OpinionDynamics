#include "od/model/DynamicsModel.hpp"

namespace od::model {

std::vector<int> initOpinions(const Graph& graph) {
    int n = graph.getNumVertices();
    std::vector<int> opinions(n, 0);

    for (int i = 0; i < n; ++i) {
        if (graph.isStubborn(i)) {
            opinions[i] = 1;
        }
    }

    return opinions;
}

double opinionChangeProbability(double x, double k1, double k2) {
    if (x <= k1) {
        return 0.0;
    } else if (x <= k2) {
        return (x - k1) / (k2 - k1);
    } else {
        return 1.0;
    }
}

static bool computeShare(
    const Graph& graph,
    int vertex,
    const std::vector<int>& opinions,
    bool useWeights,
    double& share
) {
    const auto& neighbors = graph.getNeighbors(vertex);
    if (neighbors.empty()) {
        return false;
    }

    double positive = 0.0;
    double total = 0.0;

    for (const auto& [neighborId, weight] : neighbors) {
        double contribution = useWeights ? weight : 1.0;
        total += contribution;
        if (opinions[neighborId] == 1) {
            positive += contribution;
        }
    }

    if (total <= 0.0) {
        return false;
    }

    share = positive / total;
    return true;
}

std::vector<int> stepOpinions(
    const Graph& graph,
    const std::vector<int>& currentOpinions,
    double k1,
    double k2,
    bool useWeights,
    std::mt19937& rng
) {
    std::vector<int> newOpinions = currentOpinions;
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    int n = graph.getNumVertices();

    for (int j = 0; j < n; ++j) {
        if (graph.isStubborn(j)) {
            continue;
        }

        double x = 0.0;
        if (!computeShare(graph, j, currentOpinions, useWeights, x)) {
            continue;
        }

        double prob = opinionChangeProbability(x, k1, k2);
        newOpinions[j] = (dist(rng) < prob) ? 1 : 0;
    }

    return newOpinions;
}

} // namespace od::model