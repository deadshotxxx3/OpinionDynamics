#include "od/config/ManualGenConfig.hpp"
#include "od/graph/GraphGenerator.hpp"
#include "od/io/GraphIO.hpp"
#include "od/model/DynamicsModel.hpp"
#include "od/model/EdgeEvolution.hpp"

#include <iostream>
#include <random>
#include <vector>

// Собирает базовую статистику графа для сравнения до/после save-load.
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
        if (graph.isStubborn(v)) {
            ++stats.stubbornCount;
        }
    }
    stats.numEdges = totalDegree / 2;
    stats.avgDegree = stats.numVertices > 0
        ? static_cast<double>(totalDegree) / stats.numVertices
        : 0.0;
    return stats;
}

void printStats(const std::string& label, const GraphStats& stats) {
    std::cout << "--- " << label << " ---\n";
    std::cout << "Вершин: " << stats.numVertices << "\n";
    std::cout << "Рёбер: " << stats.numEdges << "\n";
    std::cout << "Упрямых вершин: " << stats.stubbornCount << "\n";
    std::cout << "Средняя степень: " << stats.avgDegree << "\n\n";
}

int main() {
    auto cnf = od::config::collectManualGenConfig();

    auto result = od::config::validateConfig(cnf);
    if (!result.isValid) {
        std::cout << "Ошибка конфигурации: " << result.errorMessage << "\n";
        return 1;
    }
    std::cout << "Конфиг корректен.\n\n";

    od::graph::Graph graph = od::graph::generateManualGraph(cnf);

    GraphStats originalStats = computeStats(graph);
    printStats("Статистика графа (сгенерированный)", originalStats);

    // === Проверка сохранения/загрузки ===
    const std::string SAVE_FILENAME = "graph_dump.txt";

    std::cout << "=== Проверка save/load ===\n";
    try {
        od::io::saveGraph(graph, SAVE_FILENAME);
        std::cout << "Граф сохранён в файл: " << SAVE_FILENAME << "\n";

        od::graph::Graph loadedGraph = od::io::loadGraph(SAVE_FILENAME);
        std::cout << "Граф загружен обратно из файла.\n\n";

        GraphStats loadedStats = computeStats(loadedGraph);
        printStats("Статистика графа (загруженный)", loadedStats);

        bool matches = (originalStats.numVertices == loadedStats.numVertices) &&
                        (originalStats.numEdges == loadedStats.numEdges) &&
                        (originalStats.stubbornCount == loadedStats.stubbornCount);

        if (matches) {
            std::cout << "OK: статистика совпадает, save/load работает корректно.\n\n";
        } else {
            std::cout << "ОШИБКА: статистика НЕ совпадает! Проверьте saveGraph/loadGraph.\n\n";
        }
    } catch (const std::exception& e) {
        std::cout << "ОШИБКА при save/load: " << e.what() << "\n\n";
    }

    // === Симуляция (на исходном графе, как раньше) ===
    std::vector<int> opinions = od::model::initOpinions(graph);
    std::mt19937 rng(std::random_device{}());

    const int T_MAX = 500;

    std::cout << "=== Параметры симуляции ===\n";
    std::cout << "T_MAX = " << T_MAX << "\n";
    std::cout << "k1 = " << cnf.k1 << ", k2 = " << cnf.k2 << "\n";
    if (cnf.dynamicEdges) {
        std::cout << "dynamicEdges: p0 = " << cnf.p0 << ", k = " << cnf.k << "\n";
    } else {
        std::cout << "dynamicEdges: выключены\n";
    }

    int initialOnes = 0;
    for (int v : opinions) {
        if (v == 1) ++initialOnes;
    }
    std::cout << "Начальное состояние: мнение 1 у " << initialOnes
              << " (упрямых " << originalStats.stubbornCount << ")\n\n";

    std::cout << "=== Симуляция ===\n";

    for (int t = 0; t < T_MAX; ++t) {
        if (cnf.dynamicEdges) {
            double currentP = od::model::linearProbability(cnf.p0, cnf.k, t);
            od::model::evolveEdges(graph, currentP, rng);
        }

        opinions = od::model::stepOpinions(graph, opinions, cnf.k1, cnf.k2, rng);

        int ones = 0;
        for (int v : opinions) {
            if (v == 1) ++ones;
        }

        bool printThisStep = (t < 100) || (t % 50 == 0) || (t == T_MAX - 1);
        if (printThisStep) {
            std::cout << "Шаг " << (t + 1) << ": мнение 1 у " << ones
                      << " из " << graph.getNumVertices() << " вершин\n";
        }
    }

    int countOnes = 0;
    for (int v : opinions) {
        if (v == 1) ++countOnes;
    }

    std::cout << "\n=== Итог ===\n";
    std::cout << "После " << T_MAX << " шагов: мнение 1 у " << countOnes
              << " из " << graph.getNumVertices() << " вершин\n";

    long long finalEdges = 0;
    for (int v = 0; v < graph.getNumVertices(); ++v) {
        finalEdges += static_cast<long long>(graph.getNeighbors(v).size());
    }
    finalEdges /= 2;
    std::cout << "Всего рёбер в графе: " << finalEdges << "\n";

    return 0;
}