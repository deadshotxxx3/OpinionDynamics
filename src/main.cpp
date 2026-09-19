#include "od/config/ManualGenConfig.hpp"
#include "od/graph/GraphGenerator.hpp"
#include "od/model/DynamicsModel.hpp"
#include "od/model/EdgeEvolution.hpp"

#include <iostream>
#include <random>
#include <vector>

int main() {
    auto cnf = od::config::collectManualGenConfig();

    auto result = od::config::validateConfig(cnf);
    if (!result.isValid) {
        std::cout << "Ошибка конфигурации: " << result.errorMessage << "\n";
        return 1;
    }
    std::cout << "Конфиг корректен.\n\n";

    od::graph::Graph graph = od::graph::generateManualGraph(cnf);

    int numVertices = graph.getNumVertices();
    long long totalDegree = 0;
    int stubbornCount = 0;
    for (int v = 0; v < numVertices; ++v) {
        totalDegree += static_cast<long long>(graph.getNeighbors(v).size());
        if (graph.isStubborn(v)) {
            ++stubbornCount;
        }
    }
    long long numEdges = totalDegree / 2;
    double avgDegree = numVertices > 0
        ? static_cast<double>(totalDegree) / numVertices
        : 0.0;

    std::cout << "=== Статистика графа ===\n";
    std::cout << "Вершин: " << numVertices << "\n";
    std::cout << "Рёбер: " << numEdges << "\n";
    std::cout << "Упрямых вершин: " << stubbornCount << "\n";
    std::cout << "Средняя степень: " << avgDegree << "\n\n";

    const int SMALL_GRAPH_THRESHOLD = 20;
    if (numVertices <= SMALL_GRAPH_THRESHOLD) {
        std::cout << "=== Список смежности ===\n";
        for (int v = 0; v < numVertices; ++v) {
            std::cout << "Вершина " << v;
            if (graph.isStubborn(v)) {
                std::cout << " [упрямая]";
            }
            std::cout << ": ";
            const auto& neighbors = graph.getNeighbors(v);
            if (neighbors.empty()) {
                std::cout << "(нет соседей)";
            }
            for (const auto& [neighborId, weight] : neighbors) {
                std::cout << neighborId << "(w=" << weight << ") ";
            }
            std::cout << "\n";
        }
    } else {
        std::cout << "Граф слишком большой для подробного вывода (>"
                   << SMALL_GRAPH_THRESHOLD << " вершин). Показана только статистика выше.\n";
    }

    //Симуляция
    std::vector<int> opinions = od::model::initOpinions(graph);
    std::mt19937 rng(std::random_device{}());

    const int T_MAX = 500;

    std::cout << "\n=== Параметры симуляции ===\n";
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
              << " (упрямых " << stubbornCount << ")\n\n";

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