#pragma once

#include "od/graph/Graph.hpp"
#include "od/config/ManualGenConfig.hpp"
#include <random>
#include <vector>

namespace od::graph {

Graph generateManualGraph(const od::config::ManualGenConfig& config);

void assignStubbornVertices(
    Graph& graph,
    int numStubborn,
    bool manualAttach,
    const std::vector<int>& targets,
    std::mt19937& rng);

} // namespace od::graph