#pragma once

#include "od/config/ManualGenConfig.hpp"
#include "od/graph/Graph.hpp"

#include <random>
#include <vector>

namespace od::graph {

Graph generateManualGraph(const od::config::ManualGenConfig& config);

void assignStubbornVertices(
    Graph& graph,
    const od::config::StubbornGroup& assignGroup,
    const od::config::StubbornGroup& attachGroup,
    std::mt19937& rng);

} // namespace od::graph