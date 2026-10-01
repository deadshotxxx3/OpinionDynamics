#include "od/config/ManualGenConfig.hpp"
#include "od/graph/GraphGenerator.hpp"
#include "od/io/GraphIO.hpp"
#include "od/io/InputHelpers.hpp"
#include "od/io/SimulationLog.hpp"
#include "od/model/DynamicsModel.hpp"
#include "od/model/EdgeEvolution.hpp"

#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <utility>
#include <vector>

struct GraphStats {
    int numVertices = 0;
    long long numEdges = 0;
    int stubbornCount = 0;
    double avgDegree = 0.0;
};

GraphStats computeStats(const od::graph::Graph& graph) {
    GraphStats stats;
    stats.numVertices = graph.getNumVertices();

    long long totalDegree = 0;
    for (int v = 0; v < stats.numVertices; ++v) {
        totalDegree += static_cast<long long>(graph.getNeighbors(v).size());
        if (graph.isStubborn(v)) ++stats.stubbornCount;
    }
    stats.numEdges = totalDegree / 2;
    stats.avgDegree = stats.numVertices > 0
        ? static_cast<double>(totalDegree) / stats.numVertices
        : 0.0;
    return stats;
}

long long countEdges(const od::graph::Graph& graph) {
    long long totalDegree = 0;
    for (int v = 0; v < graph.getNumVertices(); ++v) {
        totalDegree += static_cast<long long>(graph.getNeighbors(v).size());
    }
    return totalDegree / 2;
}

void printStats(const std::string& label, const GraphStats& stats) {
    std::cout << "--- " << label << " ---\n";
    std::cout << "Вершин: " << stats.numVertices << "\n";
    std::cout << "Рёбер: " << stats.numEdges << "\n";
    std::cout << "Упрямых вершин: " << stats.stubbornCount << "\n";
    std::cout << "Средняя степень: " << stats.avgDegree << "\n\n";
}

struct RuntimeParams {
    double k1 = 0.0;
    double k2 = 1.0;
    bool dynamicEdges = false;
    double p0 = 0.0;
    double k = 0.0;
    bool removeEdges = false;
    double removeP0 = 0.0;
    double removeK = 0.0;
    unsigned int seed = 0;
    bool seedSpecified = false;
};

RuntimeParams collectRuntimeParamsStandalone() {
    RuntimeParams rp;

    while (true) {
        auto val = od::io::readDoubleInRange("Введите порог k1 (0.0..1.0): ", 0.0, 1.0);
        if (!val.has_value()) continue;
        rp.k1 = *val;
        break;
    }
    while (true) {
        auto val = od::io::readDoubleInRange(
            "Введите порог k2 (" + std::to_string(rp.k1) + "..1.0): ", rp.k1, 1.0);
        if (!val.has_value()) continue;
        rp.k2 = *val;
        break;
    }

    rp.dynamicEdges = od::io::readYesNo("Нужна ли динамика рёбер? Введите y/n: ");
    if (rp.dynamicEdges) {
        while (true) {
            auto val = od::io::readDoubleInRange("Введите начальную вероятность добавления p0 (0.0..1.0): ", 0.0, 1.0);
            if (!val.has_value()) continue;
            rp.p0 = *val;
            break;
        }
        while (true) {
            auto val = od::io::readDoubleInRange("Введите коэффициент добавления k (-5.0..5.0): ", -5.0, 5.0);
            if (!val.has_value()) continue;
            rp.k = *val;
            break;
        }

        rp.removeEdges = od::io::readYesNo("Нужно ли удаление рёбер? Введите y/n: ");
        if (rp.removeEdges) {
            while (true) {
                auto val = od::io::readDoubleInRange("Введите начальную вероятность удаления removeP0 (0.0..1.0): ", 0.0, 1.0);
                if (!val.has_value()) continue;
                rp.removeP0 = *val;
                break;
            }
            while (true) {
                auto val = od::io::readDoubleInRange("Введите коэффициент удаления removeK (-5.0..5.0): ", -5.0, 5.0);
                if (!val.has_value()) continue;
                rp.removeK = *val;
                break;
            }
        } else {
            rp.removeP0 = 0.0;
            rp.removeK = 0.0;
        }
    } else {
        rp.p0 = 0.0; rp.k = 0.0;
        rp.removeEdges = false; rp.removeP0 = 0.0; rp.removeK = 0.0;
    }

    bool useCustomSeed = od::io::readYesNo("Задать seed вручную? (y - задать, n - случайный): ");
    if (useCustomSeed) {
        while (true) {
            auto val = od::io::readIntInRange("Введите seed (0..2147483647): ", 0, 2147483647);
            if (!val.has_value()) continue;
            rp.seed = static_cast<unsigned int>(*val);
            rp.seedSpecified = true;
            break;
        }
    }

    return rp;
}

std::pair<od::graph::Graph, RuntimeParams> setupGraphAndParams() {
    std::cout << "Выберите способ получения графа:\n";
    std::cout << "1 - Сгенерировать новый граф\n";
    std::cout << "2 - Загрузить граф из файла\n";

    int choice;
    while (true) {
        auto val = od::io::readIntInRange("Ввод: ", 1, 2);
        if (!val.has_value()) continue;
        choice = *val;
        break;
    }

    if (choice == 1) {
        auto mode = od::config::chooseGenerationMode();
        od::config::ManualGenConfig cnf = (mode == od::config::GenerationMode::Manual)
            ? od::config::collectManualGenConfig()
            : od::config::collectGeneralGenConfig();

        auto result = od::config::validateConfig(cnf);
        if (!result.isValid) {
            std::cout << "Ошибка конфигурации: " << result.errorMessage << "\n";
            std::exit(1);
        }
        std::cout << "Конфиг корректен.\n\n";

        od::graph::Graph graph = od::graph::generateManualGraph(cnf);

        RuntimeParams rp;
        rp.k1 = cnf.k1;
        rp.k2 = cnf.k2;
        rp.dynamicEdges = cnf.dynamicEdges;
        rp.p0 = cnf.p0;
        rp.k = cnf.k;
        rp.removeEdges = cnf.removeEdges;
        rp.removeP0 = cnf.removeP0;
        rp.removeK = cnf.removeK;

        bool useCustomSeed = od::io::readYesNo("Задать seed вручную? (y - задать, n - случайный): ");
        if (useCustomSeed) {
            while (true) {
                auto val = od::io::readIntInRange("Введите seed (0..2147483647): ", 0, 2147483647);
                if (!val.has_value()) continue;
                rp.seed = static_cast<unsigned int>(*val);
                rp.seedSpecified = true;
                break;
            }
        }

        return {std::move(graph), rp};
    } else {
        std::cout << "Введите имя файла для загрузки: ";
        std::string filename;
        std::getline(std::cin, filename);
        filename = od::io::trim(filename);

        try {
            od::io::SimulationParams fileParams;
            bool paramsLoaded = false;
            od::graph::Graph graph = od::io::loadGraphWithParams(filename, fileParams, paramsLoaded);
            std::cout << "Граф успешно загружен из " << filename << "\n\n";

            RuntimeParams rp;
            if (paramsLoaded) {
                std::cout << "Параметры симуляции восстановлены из файла:\n";
                std::cout << "  seed = " << fileParams.seed << "\n";
                std::cout << "  T_MAX = " << fileParams.tMax << "\n";
                std::cout << "  k1 = " << fileParams.k1 << ", k2 = " << fileParams.k2 << "\n";
                if (fileParams.dynamicEdges) {
                    std::cout << "  dynamicEdges: p0 = " << fileParams.p0
                              << ", k = " << fileParams.k << "\n";
                    if (fileParams.removeEdges) {
                        std::cout << "  removeEdges: removeP0 = " << fileParams.removeP0
                                  << ", removeK = " << fileParams.removeK << "\n";
                    } else {
                        std::cout << "  removeEdges: выключены\n";
                    }
                } else {
                    std::cout << "  dynamicEdges: выключены\n";
                }

                bool useLoaded = od::io::readYesNo("Использовать эти параметры? (y - да, n - ввести свои): ");
                if (useLoaded) {
                    rp.k1 = fileParams.k1;
                    rp.k2 = fileParams.k2;
                    rp.dynamicEdges = fileParams.dynamicEdges;
                    rp.p0 = fileParams.p0;
                    rp.k = fileParams.k;
                    rp.removeEdges = fileParams.removeEdges;
                    rp.removeP0 = fileParams.removeP0;
                    rp.removeK = fileParams.removeK;
                    rp.seed = fileParams.seed;
                    rp.seedSpecified = true;
                } else {
                    rp = collectRuntimeParamsStandalone();
                }
            } else {
                std::cout << "В файле нет сохранённых параметров - введите их вручную.\n";
                rp = collectRuntimeParamsStandalone();
            }

            return {std::move(graph), rp};
        } catch (const std::exception& e) {
            std::cout << "Ошибка загрузки: " << e.what() << "\n";
            std::exit(1);
        }
    }
}

int main() {
    auto [graph, rp] = setupGraphAndParams();

    GraphStats stats = computeStats(graph);
    printStats("Статистика графа", stats);

    const std::string initialGraphFile = "graph_initial.txt";
    const std::string finalGraphFile   = "graph_final.txt";
    const std::string logFile          = "simulation_log.txt";

    unsigned int seed = rp.seedSpecified ? rp.seed : std::random_device{}();
    std::mt19937 rng(seed);

    const int T_MAX = 500;

    od::io::SimulationParams paramsForFile;
    paramsForFile.seed = seed;
    paramsForFile.tMax = T_MAX;
    paramsForFile.k1 = rp.k1;
    paramsForFile.k2 = rp.k2;
    paramsForFile.dynamicEdges = rp.dynamicEdges;
    paramsForFile.p0 = rp.p0;
    paramsForFile.k = rp.k;
    paramsForFile.removeEdges = rp.removeEdges;
    paramsForFile.removeP0 = rp.removeP0;
    paramsForFile.removeK = rp.removeK;

    try {
        od::io::saveGraphWithParams(graph, paramsForFile, initialGraphFile);
        std::cout << "Начальный граф сохранён в " << initialGraphFile
                  << " (вместе с параметрами)\n";
    } catch (const std::exception& e) {
        std::cout << "Ошибка сохранения начального графа: " << e.what() << "\n";
    }

    std::vector<int> opinions = od::model::initOpinions(graph);

    std::cout << "\n=== Параметры симуляции ===\n";
    std::cout << "Seed: " << seed
              << (rp.seedSpecified ? " (задан пользователем)" : " (сгенерирован)") << "\n";
    std::cout << "T_MAX = " << T_MAX << "\n";
    std::cout << "k1 = " << rp.k1 << ", k2 = " << rp.k2 << "\n";
    if (rp.dynamicEdges) {
        std::cout << "dynamicEdges: p0 = " << rp.p0 << ", k = " << rp.k << "\n";
        if (rp.removeEdges) {
            std::cout << "removeEdges: removeP0 = " << rp.removeP0
                      << ", removeK = " << rp.removeK << "\n";
        } else {
            std::cout << "removeEdges: выключены\n";
        }
    } else {
        std::cout << "dynamicEdges: выключены\n";
    }

    int initialOnes = 0;
    for (int v : opinions) if (v == 1) ++initialOnes;
    std::cout << "Начальное состояние: мнение 1 у " << initialOnes
              << " (упрямых " << stats.stubbornCount << ")\n\n";

    od::io::SimulationLog log;
    log.seed = seed;
    log.tMax = T_MAX;
    log.k1 = rp.k1;
    log.k2 = rp.k2;
    log.dynamicEdges = rp.dynamicEdges;
    log.p0 = rp.p0;
    log.k = rp.k;
    log.removeEdges = rp.removeEdges;
    log.removeP0 = rp.removeP0;
    log.removeK = rp.removeK;
    log.initialGraphFile = initialGraphFile;
    log.finalGraphFile = finalGraphFile;
    log.history.reserve(static_cast<size_t>(T_MAX));

    std::cout << "=== Симуляция ===\n";

    for (int t = 0; t < T_MAX; ++t) {
        int stepNumber = t + 1;

        if (rp.dynamicEdges) {
            double addP = od::model::linearProbability(rp.p0, rp.k, t);
            double removeP = rp.removeEdges
                ? od::model::linearProbability(rp.removeP0, rp.removeK, t)
                : 0.0;
            od::model::evolveEdges(graph, addP, removeP, rng, stepNumber, log.edgeEvents);
        }

        std::vector<int> opinionsBefore = opinions;
        opinions = od::model::stepOpinions(graph, opinions, rp.k1, rp.k2, rng);

        int ones = 0;
        for (int v = 0; v < graph.getNumVertices(); ++v) {
            if (opinions[v] == 1) ++ones;
            if (opinionsBefore[v] != opinions[v]) {
                log.opinionChanges.push_back(
                    od::io::OpinionChange{stepNumber, v, opinionsBefore[v], opinions[v]});
            }
        }

        long long edgesNow = countEdges(graph);
        log.history.push_back(od::io::StepRecord{ones, edgesNow});

        bool printThisStep = (t < 100) || (t % 50 == 0) || (t == T_MAX - 1);
        if (printThisStep) {
            std::cout << "Шаг " << stepNumber
                      << ": мнение 1 у " << ones
                      << " из " << graph.getNumVertices()
                      << " вершин, рёбер " << edgesNow << "\n";
        }
    }

    int countOnes = 0;
    for (int v : opinions) if (v == 1) ++countOnes;

    std::cout << "\n=== Итог ===\n";
    std::cout << "После " << T_MAX << " шагов: мнение 1 у " << countOnes
              << " из " << graph.getNumVertices() << " вершин\n";
    std::cout << "Всего рёбер в графе: " << countEdges(graph) << "\n";
    std::cout << "Всего изменений мнений: " << log.opinionChanges.size() << "\n";
    std::cout << "Всего событий с рёбрами: " << log.edgeEvents.size() << "\n\n";

    log.finalOpinions = opinions;

    try {
        od::io::saveGraph(graph, finalGraphFile);
        std::cout << "Конечный граф сохранён в " << finalGraphFile << "\n";
    } catch (const std::exception& e) {
        std::cout << "Ошибка сохранения конечного графа: " << e.what() << "\n";
    }

    try {
        od::io::saveSimulationLog(log, logFile);
        std::cout << "Лог симуляции сохранён в " << logFile << "\n";
    } catch (const std::exception& e) {
        std::cout << "Ошибка сохранения лога: " << e.what() << "\n";
    }

    return 0;
}