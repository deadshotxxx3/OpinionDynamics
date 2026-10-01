#pragma once

#include "od/graph/Graph.hpp"
#include <random>
#include <vector>

namespace od::model {

using od::graph::Graph;

struct EdgeEvent {
    int step;
    bool added;
    int u;
    int v;
    double weight;
};

double linearProbability(double p0, double k, int t);

void evolveEdges(
    Graph& graph,
    double addProbability,
    double removeProbability,
    std::mt19937& rng,
    int step,
    std::vector<EdgeEvent>& events);

} // namespace od::model