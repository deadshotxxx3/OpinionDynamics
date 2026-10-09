#include "od/config/ManualGenConfig.hpp"
#include "od/graph/GraphGenerator.hpp"
#include "od/io/GraphIO.hpp"
#include "od/io/InputHelpers.hpp"
#include "od/io/SimulationLog.hpp"
#include "od/model/DynamicsModel.hpp"
#include "od/model/EdgeEvolution.hpp"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <random>
#include <sstream>
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
    bool useWeights = false;
    bool dynamicEdges = false;
    double p0 = 0.0;
    double k = 0.0;
    bool removeEdges = false;
    double removeP0 = 0.0;
    double removeK = 0.0;
    unsigned int seed = 0;
    bool seedSpecified = false;
    int tMax = 500;
};

void copyFileParams(RuntimeParams& rp, const od::io::SimulationParams& fileParams) {
    rp.k1 = fileParams.k1;
    rp.k2 = fileParams.k2;
    rp.useWeights = fileParams.useWeights;
    rp.dynamicEdges = fileParams.dynamicEdges;
    rp.p0 = fileParams.p0;
    rp.k = fileParams.k;
    rp.removeEdges = fileParams.removeEdges;
    rp.removeP0 = fileParams.removeP0;
    rp.removeK = fileParams.removeK;
    rp.tMax = fileParams.tMax;
    rp.seed = fileParams.seed;
    rp.seedSpecified = true;
}

void copyConfigParams(RuntimeParams& rp, const od::config::ManualGenConfig& cnf) {
    rp.k1 = cnf.k1;
    rp.k2 = cnf.k2;
    rp.useWeights = cnf.useWeights;
    rp.dynamicEdges = cnf.dynamicEdges;
    rp.p0 = cnf.p0;
    rp.k = cnf.k;
    rp.removeEdges = cnf.removeEdges;
    rp.removeP0 = cnf.removeP0;
    rp.removeK = cnf.removeK;
}

void collectSeedSettings(RuntimeParams& rp) {
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
}

void collectTMaxSettings(RuntimeParams& rp) {
    while (true) {
        auto val = od::io::readIntInRange("Введите количество шагов симуляции (1..100000): ", 1, 100000);
        if (!val.has_value()) continue;
        rp.tMax = *val;
        break;
    }
}

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

    rp.useWeights = od::io::readYesNo("Учитывать веса рёбер при изменении мнения? Введите y/n: ");

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
    if (fileParams.dynamicEdges) {
        std::cout << "  dynamicEdges: p0 = " << fileParams.p0 << ", k = " << fileParams.k << "\n";
        if (fileParams.removeEdges) {
            std::cout << "  removeEdges: removeP0 = " << fileParams.removeP0
                      << ", removeK = " << fileParams.removeK << "\n";
        } else {
            std::cout << "  removeEdges: выключены\n";
        }
    } else {
        std::cout << "  dynamicEdges: выключены\n";
    }
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

    od::graph::Graph graph = od::graph::generateManualGraph(cnf);

    RuntimeParams rp;
    copyConfigParams(rp, cnf);
    collectTMaxSettings(rp);
    collectSeedSettings(rp);

    return {std::move(graph), rp};
}

std::pair<od::graph::Graph, RuntimeParams> loadInteractive() {
    std::cout << "Введите путь к папке прогона (например, runs/run_42): ";
    std::string dirPath;
    std::getline(std::cin, dirPath);
    dirPath = od::io::trim(dirPath);

    std::cout << "Какой граф загрузить?\n";
    std::cout << "1 - Начальный (graph_initial.txt)\n";
    std::cout << "2 - Конечный (graph_final.txt)\n";

    int graphChoice;
    while (true) {
        auto val = od::io::readIntInRange("Ввод: ", 1, 2);
        if (!val.has_value()) continue;
        graphChoice = *val;
        break;
    }

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
                copyFileParams(rp, fileParams);
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

    int choice;
    while (true) {
        auto val = od::io::readIntInRange("Ввод: ", 1, 2);
        if (!val.has_value()) continue;
        choice = *val;
        break;
    }

    return (choice == 1) ? generateInteractive() : loadInteractive();
}

struct BatchArgs {
    bool generate = true;

    std::vector<int> levelVertices;
    std::vector<double> levelProbabilities;

    bool stubborn_set = false;
    int stubborn = 0;
    std::string stubbornMode = "random";
    std::string stubbornTargets;

    std::string loadDir;
    std::string loadGraph = "initial";

    bool k1_set = false;
    double k1 = 0.0;
    bool k2_set = false;
    double k2 = 1.0;
    bool tMax_set = false;
    int tMax = 500;
    bool seed_set = false;
    unsigned int seed = 0;

    bool useWeights = false;
    bool useWeights_set = false;

    bool dynamicEdges = false;
    bool dynamicEdges_set = false;
    bool p0_set = false;
    double p0 = 0.0;
    bool k_add_set = false;
    double k = 0.0;

    bool removeEdges = false;
    bool removeEdges_set = false;
    bool removeP0_set = false;
    double removeP0 = 0.0;
    bool removeK_set = false;
    double removeK = 0.0;
};

void printBatchUsage() {
    std::cout << "Использование batch-режима:\n\n";
    std::cout << "Генерация графа:\n";
    std::cout << "  --batch --level-vertices \"10,20,15\" --level-probabilities \"0.3,0.5,0.4\"\n";
    std::cout << "  или (один уровень):\n";
    std::cout << "  --batch --vertices 100 --probability 0.3\n\n";
    std::cout << "Загрузка из папки прогона:\n";
    std::cout << "  --batch --load-dir runs/run_42 [--load-graph initial|final]\n\n";
    std::cout << "Обязательные параметры симуляции (если не подгружаются из файла):\n";
    std::cout << "  --k1 X --k2 Y [--tmax N] [--seed N]\n\n";
    std::cout << "Опции:\n";
    std::cout << "  --use-weights | --no-weights\n";
    std::cout << "  --stubborn N [--stubborn-mode random|manual] [--stubborn-targets 0,2,4]\n";
    std::cout << "  --dynamic-edges --p0 X --k Y\n";
    std::cout << "  --remove-edges --remove-p0 X --remove-k Y\n";
}

std::string nextArgValue(int argc, char** argv, int& i, const std::string& flag) {
    if (i + 1 >= argc) {
        std::cout << "Ошибка: флагу " << flag << " не хватает значения\n";
        std::exit(1);
    }
    ++i;
    return std::string(argv[i]);
}

int parseIntArg(const std::string& value, const std::string& flag) {
    try {
        return std::stoi(value);
    } catch (...) {
        std::cout << "Ошибка: некорректное число для " << flag << ": " << value << "\n";
        std::exit(1);
    }
    return 0;
}

double parseDoubleArg(const std::string& value, const std::string& flag) {
    try {
        return std::stod(value);
    } catch (...) {
        std::cout << "Ошибка: некорректное число для " << flag << ": " << value << "\n";
        std::exit(1);
    }
    return 0.0;
}

std::vector<int> parseIntList(const std::string& csv, const std::string& flag) {
    std::vector<int> result;
    std::stringstream ss(csv);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (item.empty()) continue;
        try {
            result.push_back(std::stoi(item));
        } catch (...) {
            std::cout << "Ошибка: некорректное число в " << flag << ": '" << item << "'\n";
            std::exit(1);
        }
    }
    return result;
}

std::vector<double> parseDoubleList(const std::string& csv, const std::string& flag) {
    std::vector<double> result;
    std::stringstream ss(csv);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (item.empty()) continue;
        try {
            result.push_back(std::stod(item));
        } catch (...) {
            std::cout << "Ошибка: некорректное число в " << flag << ": '" << item << "'\n";
            std::exit(1);
        }
    }
    return result;
}

void validateBatchLevels(BatchArgs& args, bool vertices_set, int single_vertices,
                         bool probability_set, double single_probability) {
    if (vertices_set && probability_set) {
        args.levelVertices = {single_vertices};
        args.levelProbabilities = {single_probability};
    }

    if (args.levelVertices.empty() || args.levelProbabilities.empty()) {
        std::cout << "Ошибка: не заданы уровни\n";
        printBatchUsage();
        std::exit(1);
    }

    if (args.levelVertices.size() != args.levelProbabilities.size()) {
        std::cout << "Ошибка: количество уровней в --level-vertices и --level-probabilities не совпадает\n";
        std::exit(1);
    }
}

BatchArgs parseBatchArgs(int argc, char** argv) {
    BatchArgs args;
    bool vertices_set = false;
    int single_vertices = 0;
    bool probability_set = false;
    double single_probability = 0.0;

    for (int i = 1; i < argc; ++i) {
        std::string flag = argv[i];

        if (flag == "--batch") {
            continue;
        } else if (flag == "--level-vertices") {
            args.levelVertices = parseIntList(nextArgValue(argc, argv, i, flag), flag);
        } else if (flag == "--level-probabilities") {
            args.levelProbabilities = parseDoubleList(nextArgValue(argc, argv, i, flag), flag);
        } else if (flag == "--vertices") {
            single_vertices = parseIntArg(nextArgValue(argc, argv, i, flag), flag);
            vertices_set = true;
        } else if (flag == "--probability") {
            single_probability = parseDoubleArg(nextArgValue(argc, argv, i, flag), flag);
            probability_set = true;
        } else if (flag == "--load-dir") {
            args.loadDir = nextArgValue(argc, argv, i, flag);
            args.generate = false;
        } else if (flag == "--load-graph") {
            args.loadGraph = nextArgValue(argc, argv, i, flag);
            if (args.loadGraph != "initial" && args.loadGraph != "final") {
                std::cout << "Ошибка: --load-graph должен быть 'initial' или 'final'\n";
                std::exit(1);
            }
        } else if (flag == "--stubborn") {
            args.stubborn = parseIntArg(nextArgValue(argc, argv, i, flag), flag);
            args.stubborn_set = true;
        } else if (flag == "--stubborn-mode") {
            args.stubbornMode = nextArgValue(argc, argv, i, flag);
        } else if (flag == "--stubborn-targets") {
            args.stubbornTargets = nextArgValue(argc, argv, i, flag);
        } else if (flag == "--k1") {
            args.k1 = parseDoubleArg(nextArgValue(argc, argv, i, flag), flag);
            args.k1_set = true;
        } else if (flag == "--k2") {
            args.k2 = parseDoubleArg(nextArgValue(argc, argv, i, flag), flag);
            args.k2_set = true;
        } else if (flag == "--tmax") {
            args.tMax = parseIntArg(nextArgValue(argc, argv, i, flag), flag);
            args.tMax_set = true;
        } else if (flag == "--seed") {
            args.seed = static_cast<unsigned int>(parseIntArg(nextArgValue(argc, argv, i, flag), flag));
            args.seed_set = true;
        } else if (flag == "--use-weights") {
            args.useWeights = true;
            args.useWeights_set = true;
        } else if (flag == "--no-weights") {
            args.useWeights = false;
            args.useWeights_set = true;
        } else if (flag == "--dynamic-edges") {
            args.dynamicEdges = true;
            args.dynamicEdges_set = true;
        } else if (flag == "--p0") {
            args.p0 = parseDoubleArg(nextArgValue(argc, argv, i, flag), flag);
            args.p0_set = true;
        } else if (flag == "--k") {
            args.k = parseDoubleArg(nextArgValue(argc, argv, i, flag), flag);
            args.k_add_set = true;
        } else if (flag == "--remove-edges") {
            args.removeEdges = true;
            args.removeEdges_set = true;
        } else if (flag == "--remove-p0") {
            args.removeP0 = parseDoubleArg(nextArgValue(argc, argv, i, flag), flag);
            args.removeP0_set = true;
        } else if (flag == "--remove-k") {
            args.removeK = parseDoubleArg(nextArgValue(argc, argv, i, flag), flag);
            args.removeK_set = true;
        } else {
            std::cout << "Неизвестный флаг: " << flag << "\n";
            printBatchUsage();
            std::exit(1);
        }
    }

    if (args.generate) {
        validateBatchLevels(args, vertices_set, single_vertices, probability_set, single_probability);
    }

    return args;
}

od::config::ManualGenConfig buildConfigFromBatchArgs(const BatchArgs& args) {
    od::config::ManualGenConfig cnf;
    cnf.cntLevels = static_cast<int>(args.levelVertices.size());

    cnf.components.clear();
    for (size_t i = 0; i < args.levelVertices.size(); ++i) {
        cnf.components.push_back(
            od::config::LevelConfig{args.levelVertices[i], args.levelProbabilities[i]});
    }

    cnf.generateStubborn = args.stubborn_set && args.stubborn > 0;
    cnf.numStubbornVertices = args.stubborn;
    cnf.stubbornManualAttach = (args.stubbornMode == "manual");
    if (cnf.stubbornManualAttach && !args.stubbornTargets.empty()) {
        cnf.stubbornTargets = parseIntList(args.stubbornTargets, "--stubborn-targets");
    }

    cnf.dynamicEdges = args.dynamicEdges;
    cnf.p0 = args.p0;
    cnf.k = args.k;
    cnf.removeEdges = args.removeEdges;
    cnf.removeP0 = args.removeP0;
    cnf.removeK = args.removeK;

    cnf.k1 = args.k1;
    cnf.k2 = args.k2;
    cnf.useWeights = args.useWeights;

    return cnf;
}

std::pair<od::graph::Graph, RuntimeParams> generateBatch(const BatchArgs& args) {
    od::config::ManualGenConfig cnf = buildConfigFromBatchArgs(args);

    auto result = od::config::validateConfig(cnf);
    if (!result.isValid) {
        std::cout << "Ошибка конфигурации: " << result.errorMessage << "\n";
        std::exit(1);
    }

    od::graph::Graph graph = od::graph::generateManualGraph(cnf);

    RuntimeParams rp;
    copyConfigParams(rp, cnf);
    rp.tMax = args.tMax;
    rp.seed = args.seed;
    rp.seedSpecified = args.seed_set;

    return {std::move(graph), rp};
}

void applyBatchOverrides(RuntimeParams& rp, const BatchArgs& args) {
    if (args.k1_set) rp.k1 = args.k1;
    if (args.k2_set) rp.k2 = args.k2;
    if (args.tMax_set) rp.tMax = args.tMax;
    if (args.seed_set) {
        rp.seed = args.seed;
        rp.seedSpecified = true;
    }
    if (args.useWeights_set) rp.useWeights = args.useWeights;
    if (args.dynamicEdges_set) rp.dynamicEdges = args.dynamicEdges;
    if (args.p0_set) rp.p0 = args.p0;
    if (args.k_add_set) rp.k = args.k;
    if (args.removeEdges_set) rp.removeEdges = args.removeEdges;
    if (args.removeP0_set) rp.removeP0 = args.removeP0;
    if (args.removeK_set) rp.removeK = args.removeK;
}

std::pair<od::graph::Graph, RuntimeParams> loadBatch(const BatchArgs& args) {
    std::string filename = args.loadDir + "/"
        + (args.loadGraph == "final" ? "graph_final.txt" : "graph_initial.txt");

    od::io::SimulationParams fileParams;
    bool paramsLoaded = false;
    od::graph::Graph graph = od::io::loadGraphWithParams(filename, fileParams, paramsLoaded);

    RuntimeParams rp;
    if (paramsLoaded) {
        copyFileParams(rp, fileParams);
    }

    applyBatchOverrides(rp, args);

    if (!paramsLoaded) {
        bool hasAll = args.k1_set && args.k2_set && args.tMax_set;
        if (!hasAll) {
            std::cout << "Ошибка: в файле нет PARAMS, нужно задать --k1, --k2, --tmax флагами\n";
            std::exit(1);
        }
    }

    return {std::move(graph), rp};
}

std::pair<od::graph::Graph, RuntimeParams> setupGraphAndParamsBatch(const BatchArgs& args) {
    try {
        return args.generate ? generateBatch(args) : loadBatch(args);
    } catch (const std::exception& e) {
        std::cout << "Ошибка: " << e.what() << "\n";
        std::exit(1);
    }
}

od::io::SimulationParams buildFileParams(const RuntimeParams& rp, unsigned int seed) {
    od::io::SimulationParams params;
    params.seed = seed;
    params.tMax = rp.tMax;
    params.k1 = rp.k1;
    params.k2 = rp.k2;
    params.useWeights = rp.useWeights;
    params.dynamicEdges = rp.dynamicEdges;
    params.p0 = rp.p0;
    params.k = rp.k;
    params.removeEdges = rp.removeEdges;
    params.removeP0 = rp.removeP0;
    params.removeK = rp.removeK;
    return params;
}

od::io::SimulationLog buildLog(const RuntimeParams& rp, unsigned int seed,
                               const std::string& initialGraphFile,
                               const std::string& finalGraphFile) {
    od::io::SimulationLog log;
    log.seed = seed;
    log.tMax = rp.tMax;
    log.k1 = rp.k1;
    log.k2 = rp.k2;
    log.useWeights = rp.useWeights;
    log.dynamicEdges = rp.dynamicEdges;
    log.p0 = rp.p0;
    log.k = rp.k;
    log.removeEdges = rp.removeEdges;
    log.removeP0 = rp.removeP0;
    log.removeK = rp.removeK;
    log.initialGraphFile = initialGraphFile;
    log.finalGraphFile = finalGraphFile;
    log.history.reserve(static_cast<size_t>(rp.tMax));
    return log;
}

void printSimulationParams(const RuntimeParams& rp, unsigned int seed, const std::string& runDir) {
    std::cout << "\n=== Параметры симуляции ===\n";
    std::cout << "Seed: " << seed << "\n";
    std::cout << "Папка прогона: " << runDir << "\n";
    std::cout << "T_MAX = " << rp.tMax << "\n";
    std::cout << "k1 = " << rp.k1 << ", k2 = " << rp.k2 << "\n";
    std::cout << "Веса рёбер: " << (rp.useWeights ? "учитываются" : "не учитываются") << "\n";
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
}

void simulationStep(od::graph::Graph& graph, std::vector<int>& opinions,
                    const RuntimeParams& rp, std::mt19937& rng, int t,
                    od::io::SimulationLog& log) {
    int stepNumber = t + 1;

    if (rp.dynamicEdges) {
        double addP = od::model::linearProbability(rp.p0, rp.k, t);
        double removeP = rp.removeEdges
            ? od::model::linearProbability(rp.removeP0, rp.removeK, t)
            : 0.0;
        od::model::evolveEdges(graph, addP, removeP, rng, stepNumber, log.edgeEvents);
    }

    std::vector<int> opinionsBefore = opinions;
    opinions = od::model::stepOpinions(graph, opinions, rp.k1, rp.k2, rp.useWeights, rng);

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

    bool printThisStep = (t < 100) || (t % 50 == 0) || (t == rp.tMax - 1);
    if (printThisStep) {
        std::cout << "Шаг " << stepNumber << ": мнение 1 у " << ones
                  << " из " << graph.getNumVertices()
                  << " вершин, рёбер " << edgesNow << "\n";
    }
}

void saveResults(const od::graph::Graph& graph, const od::io::SimulationLog& log,
                 const std::string& finalGraphFile, const std::string& logFile) {
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
}

void runSimulation(od::graph::Graph graph, RuntimeParams rp) {
    GraphStats stats = computeStats(graph);
    printStats("Статистика графа", stats);

    unsigned int seed = rp.seedSpecified ? rp.seed : std::random_device{}();

    std::string runDir = "runs/run_" + std::to_string(seed);
    std::filesystem::create_directories(runDir);

    const std::string initialGraphFile = runDir + "/graph_initial.txt";
    const std::string finalGraphFile   = runDir + "/graph_final.txt";
    const std::string logFile          = runDir + "/simulation_log.txt";

    std::mt19937 rng(seed);

    try {
        od::io::saveGraphWithParams(graph, buildFileParams(rp, seed), initialGraphFile);
        std::cout << "Начальный граф сохранён в " << initialGraphFile << "\n";
    } catch (const std::exception& e) {
        std::cout << "Ошибка сохранения начального графа: " << e.what() << "\n";
    }

    std::vector<int> opinions = od::model::initOpinions(graph);
    printSimulationParams(rp, seed, runDir);

    od::io::SimulationLog log = buildLog(rp, seed, initialGraphFile, finalGraphFile);

    std::cout << "=== Симуляция ===\n";
    for (int t = 0; t < rp.tMax; ++t) {
        simulationStep(graph, opinions, rp, rng, t, log);
    }

    int countOnes = 0;
    for (int v : opinions) if (v == 1) ++countOnes;

    std::cout << "\n=== Итог ===\n";
    std::cout << "После " << rp.tMax << " шагов: мнение 1 у " << countOnes
              << " из " << graph.getNumVertices() << " вершин\n";
    std::cout << "Всего рёбер в графе: " << countEdges(graph) << "\n";

    log.finalOpinions = opinions;
    saveResults(graph, log, finalGraphFile, logFile);
}

bool hasBatchFlag(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--batch") == 0) return true;
    }
    return false;
}

int main(int argc, char** argv) {
    if (hasBatchFlag(argc, argv)) {
        BatchArgs args = parseBatchArgs(argc, argv);
        auto [graph, rp] = setupGraphAndParamsBatch(args);
        runSimulation(std::move(graph), rp);
    } 
    else {
        auto [graph, rp] = setupGraphAndParamsInteractive();
        runSimulation(std::move(graph), rp);
    }

    return 0;
}