#include "od/config/BatchArgs.hpp"
#include "od/config/BatchMode.hpp"
#include "od/config/ManualGenConfig.hpp"
#include "od/config/RuntimeParams.hpp"
#include "od/graph/GraphGenerator.hpp"
#include "od/io/GraphIO.hpp"
#include "od/io/InputHelpers.hpp"
#include "od/model/ProbabilityFunction.hpp"
#include "od/model/Simulation.hpp"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using od::config::RuntimeParams;

double readDoubleLoop(const std::string& prompt, double minVal, double maxVal) {
    while (true) {
        auto val = od::io::readDoubleInRange(prompt, minVal, maxVal);
        if (val.has_value()) return *val;
    }
}

void collectSeedSettings(RuntimeParams& rp) {
    bool useCustomSeed = od::io::readYesNo("Задать seed вручную? (y - задать, n - случайный): ");
    if (!useCustomSeed) {
        return;
    }
    while (true) {
        auto val = od::io::readIntInRange("Введите seed (0..2147483647): ", 0, 2147483647);
        if (!val.has_value()) continue;
        rp.seed = static_cast<unsigned int>(*val);
        rp.seedSpecified = true;
        break;
    }
}

void collectTMaxSettings(RuntimeParams& rp) {
    while (true) {
        auto val = od::io::readIntInRange("Введите количество шагов симуляции (1..100000): ", 1, 100000);
        if (!val.has_value()) continue;
        rp.tMax = *val;
        break;
    }
}

void collectEdgeDynamicsStandalone(RuntimeParams& rp) {
    rp.dynamicEdges = od::io::readYesNo("Нужна ли динамика рёбер? Введите y/n: ");
    rp.removeEdges = false;

    if (!rp.dynamicEdges) {
        return;
    }

    rp.edgeAddFunction = od::config::collectProbabilityFunction("Функция вероятности ПОЯВЛЕНИЯ рёбер");

    rp.removeEdges = od::io::readYesNo("Нужно ли удаление рёбер? Введите y/n: ");
    if (rp.removeEdges) {
        rp.edgeRemoveFunction = od::config::collectProbabilityFunction("Функция вероятности ИСЧЕЗНОВЕНИЯ рёбер");
    }
}

RuntimeParams collectRuntimeParamsStandalone() {
    RuntimeParams rp;

    rp.k1 = readDoubleLoop("Введите порог k1 (0.0..1.0): ", 0.0, 1.0);
    rp.k2 = readDoubleLoop("Введите порог k2 (" + std::to_string(rp.k1) + "..1.0): ", rp.k1, 1.0);
    rp.useWeights = od::io::readYesNo("Учитывать веса рёбер при изменении мнения? Введите y/n: ");

    collectEdgeDynamicsStandalone(rp);
    collectTMaxSettings(rp);
    collectSeedSettings(rp);

    return rp;
}

void printLoadedParams(const od::io::SimulationParams& fileParams) {
    std::cout << "Параметры симуляции восстановлены из файла:\n";
    std::cout << "  seed = " << fileParams.seed << "\n";
    std::cout << "  T_MAX = " << fileParams.tMax << "\n";
    std::cout << "  k1 = " << fileParams.k1 << ", k2 = " << fileParams.k2 << "\n";
    std::cout << "  веса: " << (fileParams.useWeights ? "учитываются" : "не учитываются") << "\n";
    od::config::printEdgeDynamics(od::config::paramsFromFile(fileParams), "  ");
}

std::pair<od::graph::Graph, RuntimeParams> generateInteractive() {
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

    RuntimeParams rp = od::config::paramsFromConfig(cnf);
    collectTMaxSettings(rp);
    collectSeedSettings(rp);
    od::config::ensureSeed(rp);

    od::graph::Graph graph = od::graph::generateGraph(cnf, rp.seed);

    return {std::move(graph), rp};
}

int readChoice(const std::string& prompt, int minVal, int maxVal) {
    while (true) {
        auto val = od::io::readIntInRange(prompt, minVal, maxVal);
        if (val.has_value()) return *val;
    }
}

std::pair<od::graph::Graph, RuntimeParams> loadInteractive() {
    std::cout << "Введите путь к папке прогона (например, runs/run_42): ";
    std::string dirPath;
    std::getline(std::cin, dirPath);
    dirPath = od::io::trim(dirPath);

    std::cout << "Какой граф загрузить?\n";
    std::cout << "1 - Начальный (graph_initial.txt)\n";
    std::cout << "2 - Конечный (graph_final.txt)\n";
    int graphChoice = readChoice("Ввод: ", 1, 2);

    std::string filename = dirPath + "/" + (graphChoice == 1 ? "graph_initial.txt" : "graph_final.txt");

    try {
        od::io::SimulationParams fileParams;
        bool paramsLoaded = false;
        od::graph::Graph graph = od::io::loadGraphWithParams(filename, fileParams, paramsLoaded);
        std::cout << "Граф успешно загружен из " << filename << "\n\n";

        RuntimeParams rp;
        if (paramsLoaded) {
            printLoadedParams(fileParams);
            bool useLoaded = od::io::readYesNo("Использовать эти параметры? (y - да, n - ввести свои): ");
            if (useLoaded) {
                rp = od::config::paramsFromFile(fileParams);
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

std::pair<od::graph::Graph, RuntimeParams> setupGraphAndParamsInteractive() {
    std::cout << "Выберите способ получения графа:\n";
    std::cout << "1 - Сгенерировать новый граф\n";
    std::cout << "2 - Загрузить граф из папки прогона\n";
    int choice = readChoice("Ввод: ", 1, 2);

    return (choice == 1) ? generateInteractive() : loadInteractive();
}

int main(int argc, char** argv) {
    try {
        if (od::config::hasBatchFlag(argc, argv)) {
            od::config::SimulationSetup setup = od::config::setupBatch(od::config::parseBatchArgs(argc, argv));
            od::model::runSimulation(std::move(setup.graph), setup.params);
        } else {
            auto [graph, rp] = setupGraphAndParamsInteractive();
            od::model::runSimulation(std::move(graph), rp);
        }
    } catch (const od::config::UsageError& e) {
        std::cout << "Ошибка: " << e.what() << "\n\n";
        od::config::printBatchUsage();
        return 1;
    } catch (const std::exception& e) {
        std::cout << "Ошибка: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
