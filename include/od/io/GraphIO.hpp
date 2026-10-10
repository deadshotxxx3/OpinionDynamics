#pragma once

#include "od/graph/Graph.hpp"
#include "od/model/ProbabilityFunction.hpp"

#include <istream>
#include <ostream>
#include <string>

namespace od::io {

struct SimulationParams {
    unsigned int seed = 0;
    int tMax = 0;
    double k1 = 0.0;
    double k2 = 1.0;
    bool useWeights = false;
    bool dynamicEdges = false;
    od::model::ProbabilityFunction edgeAddFunction;
    bool removeEdges = false;
    od::model::ProbabilityFunction edgeRemoveFunction;
};

void writeFunction(std::ostream& out, const od::model::ProbabilityFunction& function);
od::model::ProbabilityFunction readFunction(std::istream& in, const std::string& source);

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