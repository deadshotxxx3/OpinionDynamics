#pragma once

#include "od/graph/Graph.hpp"
#include "od/config/ManualGenConfig.hpp"
#include <random>

namespace od::graph {
    Graph generateManualGraph(const od::config::ManualGenConfig& config);
    void assignStubbornVertices(Graph& graph, int numStubborn, std::mt19937& rng);
}