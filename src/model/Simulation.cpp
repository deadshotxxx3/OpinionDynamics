#include "od/model/Simulation.hpp"

#include "od/io/GraphIO.hpp"
#include "od/io/SimulationLog.hpp"
#include "od/model/DynamicsModel.hpp"
#include "od/model/EdgeEvolution.hpp"

#include <filesystem>
#include <iostream>
#include <random>
#include <vector>

namespace od::model {

using od::config::RuntimeParams;

namespace {

const int ALWAYS_PRINTED_STEPS = 100;
const int PRINT_EVERY = 50;

struct RunFiles {
    std::string dir;
    std::string initialGraph;
    std::string finalGraph;
    std::string log;
};

RunFiles createRunFiles(unsigned int seed) {
    const std::string base = "runs/run_" + std::to_string(seed);
    std::string dir = base;
    for (int copy = 2; std::filesystem::exists(dir); ++copy) {
        dir = base + "_" + std::to_string(copy);
    }
    std::filesystem::create_directories(dir);
    return RunFiles{dir, dir + "/graph_initial.txt", dir + "/graph_final.txt", dir + "/simulation_log.txt"};
}

int countStubborn(const od::graph::Graph& graph) {
    int count = 0;
    for (int v = 0; v < graph.getNumVertices(); ++v) {
        if (graph.isStubborn(v)) {
            ++count;
        }
    }
    return count;
}

int countOnes(const std::vector<int>& opinions) {
    int count = 0;
    for (int opinion : opinions) {
        if (opinion == 1) {
            ++count;
        }
    }
    return count;
}

void printGraphStats(const od::graph::Graph& graph) {
    int vertices = graph.getNumVertices();
    double averageDegree = vertices > 0 ? 2.0 * static_cast<double>(graph.getNumEdges()) / vertices : 0.0;

    std::cout << "--- Статистика графа ---\n"
              << "Вершин: " << vertices << "\n"
              << "Рёбер: " << graph.getNumEdges() << "\n"
              << "Упрямых вершин: " << countStubborn(graph) << "\n"
              << "Средняя степень: " << averageDegree << "\n\n";
}

void printParams(const RuntimeParams& params, const std::string& runDir) {
    std::cout << "\n=== Параметры симуляции ===\n"
              << "Seed: " << params.seed << "\n"
              << "Папка прогона: " << runDir << "\n"
              << "T_MAX = " << params.tMax << "\n"
              << "k1 = " << params.k1 << ", k2 = " << params.k2 << "\n"
              << "Веса рёбер: " << (params.useWeights ? "учитываются" : "не учитываются") << "\n";
    od::config::printEdgeDynamics(params, "");
}

od::io::SimulationLog createLog(const RuntimeParams& params, const RunFiles& files) {
    od::io::SimulationLog log;
    log.seed = params.seed;
    log.tMax = params.tMax;
    log.k1 = params.k1;
    log.k2 = params.k2;
    log.useWeights = params.useWeights;
    log.dynamicEdges = params.dynamicEdges;
    log.edgeAddFunction = params.edgeAddFunction;
    log.removeEdges = params.removeEdges;
    log.edgeRemoveFunction = params.edgeRemoveFunction;
    log.initialGraphFile = files.initialGraph;
    log.finalGraphFile = files.finalGraph;
    log.history.reserve(static_cast<size_t>(params.tMax));
    return log;
}

void evolveEdgesAt(od::graph::Graph& graph, const RuntimeParams& params, int t,
                   std::mt19937& rng, od::io::SimulationLog& log) {
    if (!params.dynamicEdges) {
        return;
    }
    double addProbability = evaluate(params.edgeAddFunction, t);
    double removeProbability = params.removeEdges ? evaluate(params.edgeRemoveFunction, t) : 0.0;
    evolveEdges(graph, addProbability, removeProbability, rng, t + 1, log.edgeEvents);
}

void recordOpinionChanges(const std::vector<int>& before, const std::vector<int>& after, int step,
                          od::io::SimulationLog& log) {
    for (size_t v = 0; v < after.size(); ++v) {
        if (before[v] != after[v]) {
            log.opinionChanges.push_back(od::io::OpinionChange{step, static_cast<int>(v), before[v], after[v]});
        }
    }
}

bool shouldPrintStep(int t, int tMax) {
    return t < ALWAYS_PRINTED_STEPS || t % PRINT_EVERY == 0 || t == tMax - 1;
}

void simulationStep(od::graph::Graph& graph, std::vector<int>& opinions, const RuntimeParams& params,
                    int t, std::mt19937& rng, od::io::SimulationLog& log) {
    const int step = t + 1;

    evolveEdgesAt(graph, params, t, rng, log);

    std::vector<int> next = stepOpinions(graph, opinions, params.k1, params.k2, params.useWeights, rng);
    recordOpinionChanges(opinions, next, step, log);
    opinions = std::move(next);

    int ones = countOnes(opinions);
    log.history.push_back(od::io::StepRecord{ones, graph.getNumEdges()});

    if (shouldPrintStep(t, params.tMax)) {
        std::cout << "Шаг " << step << ": мнение 1 у " << ones << " из " << graph.getNumVertices()
                  << " вершин, рёбер " << graph.getNumEdges() << "\n";
    }
}

template <typename SaveAction>
void trySave(const std::string& what, const std::string& path, SaveAction save) {
    try {
        save();
        std::cout << what << " сохранён в " << path << "\n";
    } catch (const std::exception& e) {
        std::cout << "Ошибка: " << what << " не сохранён: " << e.what() << "\n";
    }
}

} // namespace

void runSimulation(od::graph::Graph graph, RuntimeParams params) {
    od::config::ensureSeed(params);
    od::config::validateParams(params);

    printGraphStats(graph);

    const RunFiles files = createRunFiles(params.seed);
    trySave("Начальный граф", files.initialGraph,
            [&] { od::io::saveGraphWithParams(graph, od::config::toFileParams(params), files.initialGraph); });

    std::vector<int> opinions = initOpinions(graph);
    printParams(params, files.dir);

    od::io::SimulationLog log = createLog(params, files);
    std::mt19937 rng(params.seed);

    std::cout << "=== Симуляция ===\n";
    for (int t = 0; t < params.tMax; ++t) {
        simulationStep(graph, opinions, params, t, rng, log);
    }

    std::cout << "\n=== Итог ===\n"
              << "После " << params.tMax << " шагов: мнение 1 у " << countOnes(opinions)
              << " из " << graph.getNumVertices() << " вершин\n"
              << "Всего рёбер в графе: " << graph.getNumEdges() << "\n";

    log.finalOpinions = opinions;
    trySave("Конечный граф", files.finalGraph, [&] { od::io::saveGraph(graph, files.finalGraph); });
    trySave("Лог симуляции", files.log, [&] { od::io::saveSimulationLog(log, files.log); });
}

} // namespace od::model
