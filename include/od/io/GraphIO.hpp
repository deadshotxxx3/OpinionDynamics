#pragma once

#include "od/graph/Graph.hpp"
#include <string>

namespace od::io {

struct SimulationParams {
    unsigned int seed = 0;
    int tMax = 0;
    double k1 = 0.0;
    double k2 = 1.0;
    bool dynamicEdges = false;
    double p0 = 0.0;
    double k = 0.0;
    bool removeEdges = false;
    double removeP0 = 0.0;
    double removeK = 0.0;
    bool useWeights = false;
};

void saveGraph(const od::graph::Graph& graph, const std::string& filename);
od::graph::Graph loadGraph(const std::string& filename);

void saveGraphWithParams(
    const od::graph::Graph& graph,
    const SimulationParams& params,
    const std::string& filename);

od::graph::Graph loadGraphWithParams(
    const std::string& filename,
    SimulationParams& params,
    bool& paramsLoaded);

} // namespace od::io