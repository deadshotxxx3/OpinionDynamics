#pragma once

#include "od/graph/Graph.hpp"
#include "od/model/OpinionFunction.hpp"
#include "od/model/ProbabilityFunction.hpp"

#include <istream>
#include <ostream>
#include <string>

namespace od::io {

struct SimulationParams {
    unsigned int seed = 0;
    int tMax = 0;
    od::model::OpinionFunction opinionFunction = od::model::defaultOpinionFunction();
    bool useWeights = false;
    bool dynamicEdges = false;
    od::model::ProbabilityFunction edgeAddFunction;
    bool removeEdges = false;
    od::model::ProbabilityFunction edgeRemoveFunction;
};

void writeFunction(std::ostream& out, const od::model::ProbabilityFunction& function);
od::model::ProbabilityFunction readFunction(std::istream& in, const std::string& source);

/// @brief Пишет функцию изменения мнений в виде `имя число_параметров параметры…`.
void writeOpinionFunction(std::ostream& out, const od::model::OpinionFunction& function);

/// @brief Читает функцию изменения мнений в виде `имя число_параметров параметры…`.
/// @throws std::runtime_error Если функция не читается или некорректна.
od::model::OpinionFunction readOpinionFunction(std::istream& in, const std::string& source);

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
