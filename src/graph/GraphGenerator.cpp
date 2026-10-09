#include "od/graph/GraphGenerator.hpp"

#include <algorithm>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>

namespace od::graph {

namespace {

void validateGroupTargets(const od::config::StubbornGroup& group, int originalCount) {
    if (static_cast<int>(group.targets.size()) != group.count) {
        throw std::invalid_argument("assignStubbornVertices: размер targets не совпадает с count");
    }
    for (int idx : group.targets) {
        if (idx < 0 || idx >= originalCount) {
            throw std::out_of_range("assignStubbornVertices: индекс цели вне диапазона");
        }
    }
}

void assignExistingStubborn(
    Graph& graph,
    const od::config::StubbornGroup& group,
    int originalCount,
    std::mt19937& rng
) {
    if (group.count > originalCount) {
        throw std::invalid_argument("assignStubbornVertices: назначаемых упрямых больше, чем вершин");
    }

    if (group.manual) {
        for (int idx : group.targets) {
            graph.setStubborn(idx, true);
        }
        return;
    }

    std::vector<int> indices(static_cast<size_t>(originalCount));
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), rng);

    for (int i = 0; i < group.count; ++i) {
        graph.setStubborn(indices[static_cast<size_t>(i)], true);
    }
}

void attachNewStubborn(
    Graph& graph,
    const od::config::StubbornGroup& group,
    int originalCount,
    std::mt19937& rng
) {
    std::uniform_int_distribution<int> dist(0, originalCount - 1);
    std::uniform_real_distribution<double> weightDist(0.0, 1.0);

    for (int i = 0; i < group.count; ++i) {
        int neighbor = group.manual ? group.targets[static_cast<size_t>(i)] : dist(rng);
        int v = graph.addVertex();
        graph.addEdge(v, neighbor, weightDist(rng));
        graph.setStubborn(v, true);
    }
}

std::vector<int> computeLevelOffsets(const od::config::ManualGenConfig& config) {
    std::vector<int> offsets;
    offsets.reserve(config.components.size());
    int currentOffset = 0;
    for (const auto& level : config.components) {
        offsets.push_back(currentOffset);
        currentOffset += level.numVertices;
    }
    return offsets;
}

void generateIntraLevelEdges(
    Graph& graph,
    const od::config::ManualGenConfig& config,
    const std::vector<int>& offsets,
    std::mt19937& rng
) {
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
}

void generateInterLevelEdges(
    Graph& graph,
    const od::config::ManualGenConfig& config,
    const std::vector<int>& offsets,
    std::mt19937& rng
) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);

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
}

} // namespace

void assignStubbornVertices(
    Graph& graph,
    const od::config::StubbornGroup& assignGroup,
    const od::config::StubbornGroup& attachGroup,
    std::mt19937& rng
) {
    int originalCount = graph.getNumVertices();
    if (originalCount == 0) {
        throw std::logic_error("Нельзя назначить упрямые вершины в пустом графе");
    }

    if (assignGroup.count > 0) {
        if (assignGroup.manual) {
            validateGroupTargets(assignGroup, originalCount);
        }
        assignExistingStubborn(graph, assignGroup, originalCount, rng);
    }

    if (attachGroup.count > 0) {
        if (attachGroup.manual) {
            validateGroupTargets(attachGroup, originalCount);
        }
        attachNewStubborn(graph, attachGroup, originalCount, rng);
    }
}

Graph generateManualGraph(const od::config::ManualGenConfig& config) {
    int totalVertices = 0;
    for (const auto& level : config.components) {
        totalVertices += level.numVertices;
    }

    Graph graph(totalVertices);
    std::vector<int> offsets = computeLevelOffsets(config);

    std::mt19937 rng(std::random_device{}());

    generateIntraLevelEdges(graph, config, offsets, rng);
    generateInterLevelEdges(graph, config, offsets, rng);

    if (config.generateStubborn) {
        assignStubbornVertices(graph, config.stubbornAssign, config.stubbornAttach, rng);
    }

    return graph;
}

} // namespace od::graph