#pragma once

#include "od/graph/Graph.hpp"
#include <random>

namespace od::model {

using od::graph::Graph;

double linearProbability(double p0, double k, int t);
void evolveEdges(Graph& graph, double currentP, std::mt19937& rng);

} // namespace od::model