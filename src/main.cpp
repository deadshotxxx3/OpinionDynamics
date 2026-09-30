#include "od/config/ManualGenConfig.hpp"
#include "od/graph/GraphGenerator.hpp"
#include "od/io/GraphIO.hpp"
#include "od/io/InputHelpers.hpp"
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

struct SimulationParams {
    double k1 = 0.0;
    double k2 = 1.0;
    bool dynamicEdges = false;
    double p0 = 0.0;
    double k = 0.0;
};

SimulationParams collectSimulationParamsStandalone() {
    SimulationParams sp;

    while (true) {
        auto val = od::io::readDoubleInRange("Введите порог k1 (0.0..1.0): ", 0.0, 1.0);
        if (!val.has_value()) continue;
        sp.k1 = *val;
        break;
    }
    while (true) {
        auto val = od::io::readDoubleInRange(
            "Введите порог k2 (" + std::to_string(sp.k1) + "..1.0): ", sp.k1, 1.0);
        if (!val.has_value()) continue;
        sp.k2 = *val;
        break;
    }

    sp.dynamicEdges = od::io::readYesNo("Нужна ли динамика рёбер? Введите y/n: ");
    if (sp.dynamicEdges) {
        while (true) {
            auto val = od::io::readDoubleInRange("Введите начальную вероятность p0 (0.0..1.0): ", 0.0, 1.0);
            if (!val.has_value()) continue;
            sp.p0 = *val;
            break;
        }
        while (true) {
            auto val = od::io::readDoubleInRange("Введите коэффициент k (-5.0..5.0): ", -5.0, 5.0);
            if (!val.has_value()) continue;
            sp.k = *val;
            break;
        }
    } else {
        sp.p0 = 0.0;
        sp.k = 0.0;
    }

    return sp;
}

std::pair<od::graph::Graph, SimulationParams> setupGraphAndParams() {
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

        SimulationParams sp{cnf.k1, cnf.k2, cnf.dynamicEdges, cnf.p0, cnf.k};

        bool wantSave = od::io::readYesNo("Сохранить граф в файл? Введите y/n: ");
        if (wantSave) {
            std::cout << "Введите имя файла: ";
            std::string filename;
            std::getline(std::cin, filename);
            filename = od::io::trim(filename);
            try {
                od::io::saveGraph(graph, filename);
                std::cout << "Граф сохранён в " << filename << "\n\n";
            } catch (const std::exception& e) {
                std::cout << "Ошибка сохранения: " << e.what() << "\n\n";
            }
        }

        return {std::move(graph), sp};
    } else {
        std::cout << "Введите имя файла для загрузки: ";
        std::string filename;
        std::getline(std::cin, filename);
        filename = od::io::trim(filename);

        try {
            od::graph::Graph graph = od::io::loadGraph(filename);
            std::cout << "Граф успешно загружен из " << filename << "\n\n";

            std::cout << "Граф загружен без параметров симуляции - введите их отдельно.\n";
            SimulationParams sp = collectSimulationParamsStandalone();

            return {std::move(graph), sp};
        } catch (const std::exception& e) {
            std::cout << "Ошибка загрузки: " << e.what() << "\n";
            std::exit(1);
        }
    }
}

int main() {
    auto [graph, sp] = setupGraphAndParams();

    GraphStats stats = computeStats(graph);
    printStats("Статистика графа", stats);

    std::vector<int> opinions = od::model::initOpinions(graph);
    std::mt19937 rng(std::random_device{}());

    const int T_MAX = 500;

    std::cout << "=== Параметры симуляции ===\n";
    std::cout << "T_MAX = " << T_MAX << "\n";
    std::cout << "k1 = " << sp.k1 << ", k2 = " << sp.k2 << "\n";
    if (sp.dynamicEdges) {
        std::cout << "dynamicEdges: p0 = " << sp.p0 << ", k = " << sp.k << "\n";
    } else {
        std::cout << "dynamicEdges: выключены\n";
    }

    int initialOnes = 0;
    for (int v : opinions) {
        if (v == 1) ++initialOnes;
    }
    std::cout << "Начальное состояние: мнение 1 у " << initialOnes
              << " (упрямых " << stats.stubbornCount << ")\n\n";

    std::cout << "=== Симуляция ===\n";

    for (int t = 0; t < T_MAX; ++t) {
        if (sp.dynamicEdges) {
            double currentP = od::model::linearProbability(sp.p0, sp.k, t);
            od::model::evolveEdges(graph, currentP, rng);
        }

        opinions = od::model::stepOpinions(graph, opinions, sp.k1, sp.k2, rng);

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