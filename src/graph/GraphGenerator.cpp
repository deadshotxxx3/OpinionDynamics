#include "od/graph/GraphGenerator.hpp"

#include <algorithm>
#include <numeric>
#include <vector>

namespace od::graph {

void assignStubbornVertices(Graph& graph, int numStubborn, std::mt19937& rng) {
    int totalVertices = graph.getNumVertices();
    if (numStubborn <= 0 || numStubborn > totalVertices) {
        return;
    }

    std::vector<int> indices(totalVertices);
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), rng);

    for (int i = 0; i < numStubborn; ++i) {
        graph.setStubborn(indices[i], true);
    }
}

Graph generateManualGraph(const od::config::ManualGenConfig& config) {
    int totalVertices = 0;
    for (const auto& level : config.components) {
        totalVertices += level.numVertices;
    }

    Graph graph(totalVertices);

    std::vector<int> offsets;
    offsets.reserve(config.components.size());
    int currentOffset = 0;
    for (const auto& level : config.components) {
        offsets.push_back(currentOffset);
        currentOffset += level.numVertices;
    }

    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    for (size_t i = 0; i < config.components.size(); ++i) {
        const auto& level = config.components[i];
        int offset = offsets[i];
        double p = level.probability;

        for (int u = 0; u < level.numVertices; ++u) {
            for (int v = u + 1; v < level.numVertices; ++v) {
                if (dist(rng) < p) {
                    graph.addEdge(offset + u, offset + v, dist(rng));
                }
            }
        }
    }

    for (size_t i = 0; i < config.components.size(); ++i) {
        for (size_t j = i + 1; j < config.components.size(); ++j) {
            const auto& levelI = config.components[i];
            const auto& levelJ = config.components[j];
            int offsetI = offsets[i];
            int offsetJ = offsets[j];
            double p = std::min(levelI.probability, levelJ.probability);

            for (int u = 0; u < levelI.numVertices; ++u) {
                for (int v = 0; v < levelJ.numVertices; ++v) {
                    if (dist(rng) < p) {
                        graph.addEdge(offsetI + u, offsetJ + v, dist(rng));
                    }
                }
            }
        }
    }

    if (config.generateStubborn) {
        assignStubbornVertices(graph, config.numStubbornVertices, rng);
    }

    return graph;
}

}