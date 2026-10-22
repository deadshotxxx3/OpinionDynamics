#include "od/model/DynamicsModel.hpp"

namespace od::model {

namespace {

int chooseOpinion(const std::vector<double>& probabilities, const std::vector<int>& order,
                  int currentOpinion, double randomValue) {
    double cumulative = 0.0;
    bool anyPositive = false;
    for (int opinion : order) {
        double p = probabilities[static_cast<size_t>(opinion)];
        anyPositive = anyPositive || p > 0.0;
        cumulative += p;
        if (randomValue < cumulative) {
            return opinion;
        }
    }
    return anyPositive ? order.back() : currentOpinion;
}

} // namespace

std::vector<int> initOpinions(const Graph& graph) {
    int n = graph.getNumVertices();
    std::vector<int> opinions(static_cast<size_t>(n), 0);
    for (int i = 0; i < n; ++i) {
        if (graph.isStubborn(i)) {
            opinions[static_cast<size_t>(i)] = 1;
        }
    }
    return opinions;
}

std::vector<double> neighborShares(const Graph& graph, int vertex, const std::vector<int>& opinions,
                                   int numOpinions, bool useWeights) {
    const Graph::Neighbors& neighbors = graph.getNeighbors(vertex);
    if (neighbors.empty()) {
        return {};
    }

    std::vector<double> weightByOpinion(static_cast<size_t>(numOpinions), 0.0);
    double total = 0.0;
    for (const auto& [neighbor, weight] : neighbors) {
        double contribution = useWeights ? weight : 1.0;
        total += contribution;
        weightByOpinion[static_cast<size_t>(opinions[static_cast<size_t>(neighbor)])] += contribution;
    }
    if (total <= 0.0) {
        return {};
    }

    for (double& share : weightByOpinion) {
        share /= total;
    }
    return weightByOpinion;
}

std::vector<int> stepOpinions(const Graph& graph, const std::vector<int>& currentOpinions,
                              const OpinionRule& rule, bool useWeights, std::mt19937& rng) {
    std::vector<int> newOpinions = currentOpinions;
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    const std::vector<int> order = samplingOrder(rule);
    const int numOpinions = static_cast<int>(rule.size());

    for (int v = 0; v < graph.getNumVertices(); ++v) {
        if (graph.isStubborn(v)) {
            continue;
        }
        std::vector<double> shares = neighborShares(graph, v, currentOpinions, numOpinions, useWeights);
        if (shares.empty()) {
            continue;
        }
        std::vector<double> probabilities = adoptionProbabilities(rule, shares);
        newOpinions[static_cast<size_t>(v)] =
            chooseOpinion(probabilities, order, currentOpinions[static_cast<size_t>(v)], unit(rng));
    }
    return newOpinions;
}

} // namespace od::model
