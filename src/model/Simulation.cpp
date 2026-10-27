#include "od/model/Simulation.hpp"

#include "od/io/GraphIO.hpp"
#include "od/io/SimulationLog.hpp"
#include "od/model/DynamicsModel.hpp"
#include "od/model/EdgeEvolution.hpp"
#include "od/model/InitialOpinions.hpp"

#include <filesystem>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
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

RunFiles createRunFiles(const std::string& runsDir, unsigned int seed) {
    const std::string base = runsDir + "/run_" + std::to_string(seed);
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

const size_t SUMMARY_OPINIONS = 5;

std::string describeLeader(const std::vector<OpinionCount>& counts, int numVertices) {
    if (counts.empty()) {
        return "вершин нет";
    }
    return "мнений " + std::to_string(counts.size()) + ", чаще всего " + std::to_string(counts.front().opinion)
           + " у " + std::to_string(counts.front().count) + " из " + std::to_string(numVertices) + " вершин";
}

std::string describeCounts(const std::vector<OpinionCount>& counts) {
    std::string text;
    for (size_t i = 0; i < counts.size() && i < SUMMARY_OPINIONS; ++i) {
        if (i > 0) text += ", ";
        text += std::to_string(counts[i].opinion) + " - " + std::to_string(counts[i].count);
    }
    if (counts.size() > SUMMARY_OPINIONS) {
        text += " и ещё " + std::to_string(counts.size() - SUMMARY_OPINIONS) + " мнений";
    }
    return text;
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
              << "Веса рёбер: " << (params.useWeights ? "учитываются" : "не учитываются") << "\n";
    od::config::printOpinionModel(params, "");
    od::config::printEdgeDynamics(params, "");
}

od::io::SimulationLog createLog(const RuntimeParams& params, const RunFiles& files) {
    od::io::SimulationLog log;
    log.params = od::config::toFileParams(params);
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
                    OpinionUpdater& updater, int t, std::mt19937& rng, od::io::SimulationLog& log, bool quiet) {
    const int step = t + 1;

    evolveEdgesAt(graph, params, t, rng, log);

    std::vector<int> next = updater.step(graph, opinions, rng);
    recordOpinionChanges(opinions, next, step, log);
    opinions = std::move(next);

    std::vector<OpinionCount> counts = countOpinions(opinions, params.numOpinions);
    if (!quiet && shouldPrintStep(t, params.tMax)) {
        std::cout << "Шаг " << step << ": " << describeLeader(counts, graph.getNumVertices())
                  << ", рёбер " << graph.getNumEdges() << "\n";
    }
    log.history.push_back(od::io::StepRecord{graph.getNumEdges(), std::move(counts)});
}

template <typename SaveAction>
void trySave(const std::string& what, const std::string& path, bool quiet, SaveAction save) {
    try {
        save();
        if (!quiet) {
            std::cout << what << " сохранён в " << path << "\n";
        }
    } catch (const std::exception& e) {
        std::cout << "Ошибка: " << what << " не сохранён: " << e.what() << "\n";
    }
}

std::vector<int> prepareOpinions(const od::graph::Graph& graph, const RuntimeParams& params,
                                 std::vector<int> given, bool quiet) {
    if (given.empty()) {
        return makeInitialOpinions(graph, params.initialOpinions, params.numOpinions, params.seed);
    }
    std::string error = validateOpinionVector(given, graph.getNumVertices(), params.numOpinions);
    if (!error.empty()) {
        throw std::invalid_argument("начальные мнения из файла не подходят: " + error
                                    + ". Увеличьте число мнений или задайте начальные мнения заново");
    }
    if (!quiet) {
        std::cout << "Начальные мнения взяты из файла графа\n";
    }
    return given;
}

} // namespace

void runSimulation(od::graph::Graph graph, RuntimeParams params, std::vector<int> initialOpinions,
                   const od::config::OutputOptions& output) {
    const bool quiet = output.quiet;
    od::config::ensureSeed(params);
    od::config::validateParams(params);

    if (!quiet) {
        printGraphStats(graph);
    }
    std::vector<int> opinions = prepareOpinions(graph, params, std::move(initialOpinions), quiet);

    const RunFiles files = createRunFiles(output.runsDir, params.seed);
    trySave("Начальный граф", files.initialGraph, quiet, [&] {
        od::io::saveGraphWithParams(graph, opinions, od::config::toFileParams(params), files.initialGraph);
    });

    if (quiet) {
        std::cout << "Папка прогона: " << files.dir << "\n";
    } else {
        printParams(params, files.dir);
        std::cout << "Начало: " << describeCounts(countOpinions(opinions, params.numOpinions)) << "\n";
    }

    od::io::SimulationLog log = createLog(params, files);
    std::mt19937 rng(params.seed);
    OpinionUpdater updater(makeRule(params.opinionFunction, params.numOpinions, params.choiceRule),
                           ChoiceSettings{params.choiceRule, params.popularity}, params.useWeights);

    if (!quiet) {
        std::cout << "=== Симуляция ===\n";
    }
    for (int t = 0; t < params.tMax; ++t) {
        simulationStep(graph, opinions, params, updater, t, rng, log, quiet);
    }

    const std::vector<OpinionCount> finalCounts = countOpinions(opinions, params.numOpinions);
    if (!quiet) {
        std::cout << "\n=== Итог ===\n";
    }
    std::cout << "После " << params.tMax << " шагов: " << describeLeader(finalCounts, graph.getNumVertices()) << "\n";
    if (!quiet) {
        std::cout << "Мнения по числу вершин: " << describeCounts(finalCounts) << "\n"
                  << "Всего рёбер в графе: " << graph.getNumEdges() << "\n";
    }

    log.finalOpinions = opinions;
    trySave("Конечный граф", files.finalGraph, quiet, [&] { od::io::saveGraph(graph, opinions, files.finalGraph); });
    trySave("Лог симуляции", files.log, quiet, [&] { od::io::saveSimulationLog(log, files.log); });
}

} // namespace od::model
