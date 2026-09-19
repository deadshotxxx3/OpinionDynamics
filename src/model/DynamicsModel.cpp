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

std::vector<int> stepOpinions(
    const Graph& graph,
    const std::vector<int>& currentOpinions,
    double k1,
    double k2,
    std::mt19937& rng
) {
    std::vector<int> newOpinions = currentOpinions;
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    int n = graph.getNumVertices();

    for (int j = 0; j < n; ++j) {
        if (graph.isStubborn(j)) {
            continue;
        }

        const auto& neighbors = graph.getNeighbors(j);

        if (neighbors.empty()) {
            continue;
        }

        int c_j = 0;
        for (const auto& [neighborId, weight] : neighbors) {
            c_j += currentOpinions[neighborId];
        }

        int d_j = static_cast<int>(neighbors.size());
        double x = static_cast<double>(c_j) / d_j;

        double prob = opinionChangeProbability(x, k1, k2);

        if (dist(rng) < prob) {
            newOpinions[j] = 1;
        } else {
            newOpinions[j] = 0;
        }
    }

    return newOpinions;
}

} // namespace od::model