#pragma once

#include "od/graph/Graph.hpp"
#include <vector>
#include <random>

namespace od::model {

using od::graph::Graph;

std::vector<int> initOpinions(const Graph& graph);
double opinionChangeProbability(double x, double k1, double k2);

std::vector<int> stepOpinions(
    const Graph& graph,
    const std::vector<int>& currentOpinions,
    double k1,
    double k2,
    bool useWeights,
    std::mt19937& rng
);

} // namespace od::model