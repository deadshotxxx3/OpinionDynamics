#include "od/graph/GraphGenerator.hpp"

#include <algorithm>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace od::graph {

namespace {

using od::config::ManualGenConfig;
using od::config::StubbornGroup;

constexpr unsigned int GRAPH_RANDOM_STREAM = 1;

struct LevelRange {
    int begin;
    int end;
    double probability;
};

std::mt19937 makeGraphRng(unsigned int seed) {
    std::seed_seq sequence{seed, GRAPH_RANDOM_STREAM};
    return std::mt19937(sequence);
}

std::vector<LevelRange> computeLevelRanges(const ManualGenConfig& config) {
    std::vector<LevelRange> ranges;
    ranges.reserve(config.components.size());

    int begin = 0;
    for (const auto& level : config.components) {
        ranges.push_back(LevelRange{begin, begin + level.numVertices, level.probability});
        begin += level.numVertices;
    }
    return ranges;
}

void connectWithProbability(Graph& graph, int u, int v, double probability, std::mt19937& rng) {
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    if (unit(rng) < probability) {
        graph.addEdge(u, v, unit(rng));
    }
}

void addEdgesInsideLevel(Graph& graph, const LevelRange& level, std::mt19937& rng) {
    for (int u = level.begin; u < level.end; ++u) {
        for (int v = u + 1; v < level.end; ++v) {
            connectWithProbability(graph, u, v, level.probability, rng);
        }
    }
}

void addEdgesBetweenLevels(Graph& graph, const LevelRange& first, const LevelRange& second,
                           std::mt19937& rng) {
    double probability = std::min(first.probability, second.probability);
    for (int u = first.begin; u < first.end; ++u) {
        for (int v = second.begin; v < second.end; ++v) {
            connectWithProbability(graph, u, v, probability, rng);
        }
    }
}

void addLevelEdges(Graph& graph, const std::vector<LevelRange>& levels, std::mt19937& rng) {
    for (const auto& level : levels) {
        addEdgesInsideLevel(graph, level, rng);
    }
    for (size_t i = 0; i < levels.size(); ++i) {
        for (size_t j = i + 1; j < levels.size(); ++j) {
            addEdgesBetweenLevels(graph, levels[i], levels[j], rng);
        }
    }
}

void checkManualTargets(const StubbornGroup& group, int levelVertices, const std::string& title) {
    if (static_cast<int>(group.targets.size()) != group.count) {
        throw std::invalid_argument(title + ": указано " + std::to_string(group.targets.size())
                                    + " вершин, а нужно " + std::to_string(group.count));
    }
    for (int target : group.targets) {
        if (target < 0 || target >= levelVertices) {
            throw std::out_of_range(title + ": вершина " + std::to_string(target)
                                    + " вне диапазона [0, " + std::to_string(levelVertices) + ")");
        }
    }
}

std::vector<int> pickDistinctVertices(int count, int levelVertices, std::mt19937& rng) {
    std::vector<int> vertices(static_cast<size_t>(levelVertices));
    std::iota(vertices.begin(), vertices.end(), 0);
    std::shuffle(vertices.begin(), vertices.end(), rng);
    vertices.resize(static_cast<size_t>(count));
    return vertices;
}

void assignExistingStubborn(Graph& graph, const StubbornGroup& group, int levelVertices,
                            std::mt19937& rng) {
    const std::string title = "Назначение упрямых";
    if (group.count > levelVertices) {
        throw std::invalid_argument(title + ": " + std::to_string(group.count)
                                    + " упрямых при " + std::to_string(levelVertices) + " вершинах");
    }

    std::vector<int> vertices;
    if (group.manual) {
        checkManualTargets(group, levelVertices, title);
        vertices = group.targets;
    } else {
        vertices = pickDistinctVertices(group.count, levelVertices, rng);
    }

    for (int vertex : vertices) {
        graph.setStubborn(vertex, true);
    }
}

void attachNewStubborn(Graph& graph, const StubbornGroup& group, int levelVertices,
                       std::mt19937& rng) {
    if (group.manual) {
        checkManualTargets(group, levelVertices, "Прикрепление упрямых");
    }

    std::uniform_int_distribution<int> anyVertex(0, levelVertices - 1);
    std::uniform_real_distribution<double> unit(0.0, 1.0);

    for (int i = 0; i < group.count; ++i) {
        int target = group.manual ? group.targets[static_cast<size_t>(i)] : anyVertex(rng);
        int stubborn = graph.addVertex();
        graph.addEdge(stubborn, target, unit(rng));
        graph.setStubborn(stubborn, true);
    }
}

void addStubbornVertices(Graph& graph, const ManualGenConfig& config, std::mt19937& rng) {
    if (!config.generateStubborn) {
        return;
    }

    int levelVertices = graph.getNumVertices();
    if (levelVertices == 0) {
        throw std::invalid_argument("Нельзя расставить упрямые вершины в пустом графе");
    }

    if (config.stubbornAssign.count > 0) {
        assignExistingStubborn(graph, config.stubbornAssign, levelVertices, rng);
    }
    if (config.stubbornAttach.count > 0) {
        attachNewStubborn(graph, config.stubbornAttach, levelVertices, rng);
    }
}

} // namespace

Graph generateGraph(const ManualGenConfig& config, unsigned int seed) {
    std::vector<LevelRange> levels = computeLevelRanges(config);
    int levelVertices = levels.empty() ? 0 : levels.back().end;

    Graph graph(levelVertices);
    std::mt19937 rng = makeGraphRng(seed);

    addLevelEdges(graph, levels, rng);
    addStubbornVertices(graph, config, rng);

    return graph;
}

} // namespace od::graph