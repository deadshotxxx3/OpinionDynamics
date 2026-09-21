#pragma once

#include "od/graph/Graph.hpp"
#include <string>

namespace od::io {

void saveGraph(const od::graph::Graph& graph, const std::string& filename);
od::graph::Graph loadGraph(const std::string& filename);

} // namespace od::io