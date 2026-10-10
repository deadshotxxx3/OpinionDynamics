#include "od/config/ManualGenConfig.hpp"
#include "od/graph/GraphGenerator.hpp"
#include "od/io/GraphIO.hpp"
#include "od/io/InputHelpers.hpp"
#include "od/io/SimulationLog.hpp"
#include "od/model/DynamicsModel.hpp"
#include "od/model/EdgeEvolution.hpp"
#include "od/model/ProbabilityFunction.hpp"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <optional>
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
    stats.numEdges = graph.getNumEdges();

    for (int v = 0; v < stats.numVertices; ++v) {
        if (graph.isStubborn(v)) ++stats.stubbornCount;
    }
    stats.avgDegree = stats.numVertices > 0
        ? 2.0 * static_cast<double>(stats.numEdges) / stats.numVertices
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

struct RuntimeParams {
    double k1 = 0.0;
    double k2 = 1.0;
    bool useWeights = false;
    bool dynamicEdges = false;
    od::model::ProbabilityFunction edgeAddFunction;
    bool removeEdges = false;
    od::model::ProbabilityFunction edgeRemoveFunction;
    unsigned int seed = 0;
    bool seedSpecified = false;
    int tMax = 500;
};

void copyFileParams(RuntimeParams& rp, const od::io::SimulationParams& fileParams) {
    rp.k1 = fileParams.k1;
    rp.k2 = fileParams.k2;
    rp.useWeights = fileParams.useWeights;
    rp.dynamicEdges = fileParams.dynamicEdges;
    rp.edgeAddFunction = fileParams.edgeAddFunction;
    rp.removeEdges = fileParams.removeEdges;
    rp.edgeRemoveFunction = fileParams.edgeRemoveFunction;
    rp.tMax = fileParams.tMax;
    rp.seed = fileParams.seed;
    rp.seedSpecified = true;
}

void copyConfigParams(RuntimeParams& rp, const od::config::ManualGenConfig& cnf) {
    rp.k1 = cnf.k1;
    rp.k2 = cnf.k2;
    rp.useWeights = cnf.useWeights;
    rp.dynamicEdges = cnf.dynamicEdges;
    rp.edgeAddFunction = cnf.edgeAddFunction;
    rp.removeEdges = cnf.removeEdges;
    rp.edgeRemoveFunction = cnf.edgeRemoveFunction;
}

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

void printEdgeDynamics(const std::string& indent, bool dynamicEdges,
                       const od::model::ProbabilityFunction& addFunction,
                       bool removeEdges,
                       const od::model::ProbabilityFunction& removeFunction) {
    if (!dynamicEdges) {
        std::cout << indent << "Динамика рёбер: выключена\n";
        return;
    }
    std::cout << indent << "Появление рёбер: " << od::model::describeFunction(addFunction) << "\n";
    if (removeEdges) {
        std::cout << indent << "Исчезновение рёбер: " << od::model::describeFunction(removeFunction) << "\n";
    } else {
        std::cout << indent << "Исчезновение рёбер: выключено\n";
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
    printEdgeDynamics("  ", fileParams.dynamicEdges, fileParams.edgeAddFunction,
                      fileParams.removeEdges, fileParams.edgeRemoveFunction);
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
    int choice = readChoice("Ввод: ", 1, 2);

    return (choice == 1) ? generateInteractive() : loadInteractive();
}

struct BatchArgs {
    bool generate = true;

    std::vector<int> levelVertices;
    std::vector<double> levelProbabilities;

    int attachCount = 0;
    std::string attachMode = "random";
    std::string attachTargets;

    int assignCount = 0;
    std::string assignMode = "random";
    std::string assignTargets;

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
    std::string addFuncName;
    std::string addParams;
    bool addLegacyA_set = false;
    double addLegacyA = 0.0;
    bool addLegacyB_set = false;
    double addLegacyB = 0.0;

    bool removeEdges = false;
    bool removeEdges_set = false;
    std::string removeFuncName;
    std::string removeParams;
    bool removeLegacyA_set = false;
    double removeLegacyA = 0.0;
    bool removeLegacyB_set = false;
    double removeLegacyB = 0.0;
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
    std::cout << "  --stubborn-attach N [--stubborn-attach-mode random|manual] [--stubborn-attach-targets 0,2,4]\n";
    std::cout << "  --stubborn-assign N [--stubborn-assign-mode random|manual] [--stubborn-assign-targets 1,3,5]\n";
    std::cout << "  --dynamic-edges --add-func NAME --add-params \"a,b,...\"\n";
    std::cout << "  --remove-edges --remove-func NAME --remove-params \"a,b,...\"\n";
    std::cout << "  (устаревшее: --p0 X --k Y и --remove-p0 X --remove-k Y задают линейную функцию)\n\n";
    std::cout << "Функции:\n";
    for (od::model::FunctionKind kind : od::model::allKinds()) {
        std::cout << "  " << od::model::kindToString(kind) << " (" << od::model::paramsDescription(kind)
                  << ") - " << od::model::kindDisplayName(kind) << "\n";
    }
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

std::string parseModeArg(const std::string& value, const std::string& flag) {
    if (value != "random" && value != "manual") {
        std::cout << "Ошибка: " << flag << " должен быть 'random' или 'manual'\n";
        std::exit(1);
    }
    return value;
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

bool parseStubbornFlag(const std::string& flag, int argc, char** argv, int& i, BatchArgs& args) {
    if (flag == "--stubborn" || flag == "--stubborn-attach") {
        args.attachCount = parseIntArg(nextArgValue(argc, argv, i, flag), flag);
    } else if (flag == "--stubborn-mode" || flag == "--stubborn-attach-mode") {
        args.attachMode = parseModeArg(nextArgValue(argc, argv, i, flag), flag);
    } else if (flag == "--stubborn-targets" || flag == "--stubborn-attach-targets") {
        args.attachTargets = nextArgValue(argc, argv, i, flag);
    } else if (flag == "--stubborn-assign") {
        args.assignCount = parseIntArg(nextArgValue(argc, argv, i, flag), flag);
    } else if (flag == "--stubborn-assign-mode") {
        args.assignMode = parseModeArg(nextArgValue(argc, argv, i, flag), flag);
    } else if (flag == "--stubborn-assign-targets") {
        args.assignTargets = nextArgValue(argc, argv, i, flag);
    } else {
        return false;
    }
    return true;
}

bool parseEdgeDynamicsFlag(const std::string& flag, int argc, char** argv, int& i, BatchArgs& args) {
    if (flag == "--dynamic-edges") {
        args.dynamicEdges = true;
        args.dynamicEdges_set = true;
    } else if (flag == "--add-func") {
        args.addFuncName = nextArgValue(argc, argv, i, flag);
    } else if (flag == "--add-params") {
        args.addParams = nextArgValue(argc, argv, i, flag);
    } else if (flag == "--p0") {
        args.addLegacyA = parseDoubleArg(nextArgValue(argc, argv, i, flag), flag);
        args.addLegacyA_set = true;
    } else if (flag == "--k") {
        args.addLegacyB = parseDoubleArg(nextArgValue(argc, argv, i, flag), flag);
        args.addLegacyB_set = true;
    } else if (flag == "--remove-edges") {
        args.removeEdges = true;
        args.removeEdges_set = true;
    } else if (flag == "--remove-func") {
        args.removeFuncName = nextArgValue(argc, argv, i, flag);
    } else if (flag == "--remove-params") {
        args.removeParams = nextArgValue(argc, argv, i, flag);
    } else if (flag == "--remove-p0") {
        args.removeLegacyA = parseDoubleArg(nextArgValue(argc, argv, i, flag), flag);
        args.removeLegacyA_set = true;
    } else if (flag == "--remove-k") {
        args.removeLegacyB = parseDoubleArg(nextArgValue(argc, argv, i, flag), flag);
        args.removeLegacyB_set = true;
    } else {
        return false;
    }
    return true;
}

bool parseSimulationFlag(const std::string& flag, int argc, char** argv, int& i, BatchArgs& args) {
    if (flag == "--k1") {
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
    } else {
        return false;
    }
    return true;
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
        } else if (parseStubbornFlag(flag, argc, argv, i, args)) {
            continue;
        } else if (parseEdgeDynamicsFlag(flag, argc, argv, i, args)) {
            continue;
        } else if (parseSimulationFlag(flag, argc, argv, i, args)) {
            continue;
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

od::config::StubbornGroup buildStubbornGroup(int count, const std::string& mode,
                                             const std::string& targets, const std::string& flag) {
    od::config::StubbornGroup group;
    group.count = count;
    group.manual = (mode == "manual");
    if (group.manual && count > 0) {
        if (targets.empty()) {
            std::cout << "Ошибка: ручной режим, но не задан " << flag << "\n";
            std::exit(1);
        }
        group.targets = parseIntList(targets, flag);
    }
    return group;
}

std::optional<od::model::ProbabilityFunction> buildBatchFunction(
    const std::string& name,
    const std::string& paramsCsv,
    bool legacyASet, double legacyA,
    bool legacyBSet, double legacyB,
    const std::string& prefix
) {
    const std::string funcFlag = "--" + prefix + "-func";
    const std::string paramsFlag = "--" + prefix + "-params";

    if (!name.empty()) {
        auto kind = od::model::kindFromString(name);
        if (!kind.has_value()) {
            std::cout << "Ошибка: неизвестная функция в " << funcFlag << ": " << name << "\n";
            printBatchUsage();
            std::exit(1);
        }

        od::model::ProbabilityFunction function;
        function.kind = *kind;
        function.params = parseDoubleList(paramsCsv, paramsFlag);

        std::string error = od::model::validateFunction(function);
        if (!error.empty()) {
            std::cout << "Ошибка в " << paramsFlag << ": " << error << "\n";
            std::exit(1);
        }
        return function;
    }

    if (!paramsCsv.empty()) {
        std::cout << "Ошибка: " << paramsFlag << " задан без " << funcFlag << "\n";
        std::exit(1);
    }

    if (legacyASet || legacyBSet) {
        od::model::ProbabilityFunction function;
        function.kind = od::model::FunctionKind::Linear;
        function.params = {legacyA, legacyB};
        return function;
    }

    return std::nullopt;
}

std::optional<od::model::ProbabilityFunction> buildAddFunction(const BatchArgs& args) {
    return buildBatchFunction(args.addFuncName, args.addParams,
                              args.addLegacyA_set, args.addLegacyA,
                              args.addLegacyB_set, args.addLegacyB, "add");
}

std::optional<od::model::ProbabilityFunction> buildRemoveFunction(const BatchArgs& args) {
    return buildBatchFunction(args.removeFuncName, args.removeParams,
                              args.removeLegacyA_set, args.removeLegacyA,
                              args.removeLegacyB_set, args.removeLegacyB, "remove");
}

od::config::ManualGenConfig buildConfigFromBatchArgs(const BatchArgs& args) {
    od::config::ManualGenConfig cnf;
    cnf.cntLevels = static_cast<int>(args.levelVertices.size());

    for (size_t i = 0; i < args.levelVertices.size(); ++i) {
        cnf.components.push_back(
            od::config::LevelConfig{args.levelVertices[i], args.levelProbabilities[i]});
    }

    cnf.stubbornAssign = buildStubbornGroup(
        args.assignCount, args.assignMode, args.assignTargets, "--stubborn-assign-targets");
    cnf.stubbornAttach = buildStubbornGroup(
        args.attachCount, args.attachMode, args.attachTargets, "--stubborn-attach-targets");
    cnf.generateStubborn = cnf.stubbornAssign.count > 0 || cnf.stubbornAttach.count > 0;

    std::optional<od::model::ProbabilityFunction> addFunction = buildAddFunction(args);
    std::optional<od::model::ProbabilityFunction> removeFunction = buildRemoveFunction(args);

    cnf.dynamicEdges = args.dynamicEdges;
    if (cnf.dynamicEdges) {
        if (!addFunction.has_value()) {
            std::cout << "Ошибка: --dynamic-edges требует --add-func и --add-params\n";
            std::exit(1);
        }
        cnf.edgeAddFunction = *addFunction;
    }

    cnf.removeEdges = args.removeEdges;
    if (cnf.removeEdges) {
        if (!removeFunction.has_value()) {
            std::cout << "Ошибка: --remove-edges требует --remove-func и --remove-params\n";
            std::exit(1);
        }
        cnf.edgeRemoveFunction = *removeFunction;
    }

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
    if (args.removeEdges_set) rp.removeEdges = args.removeEdges;

    std::optional<od::model::ProbabilityFunction> addFunction = buildAddFunction(args);
    if (addFunction.has_value()) rp.edgeAddFunction = *addFunction;

    std::optional<od::model::ProbabilityFunction> removeFunction = buildRemoveFunction(args);
    if (removeFunction.has_value()) rp.edgeRemoveFunction = *removeFunction;
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
    params.edgeAddFunction = rp.edgeAddFunction;
    params.removeEdges = rp.removeEdges;
    params.edgeRemoveFunction = rp.edgeRemoveFunction;
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
    log.edgeAddFunction = rp.edgeAddFunction;
    log.removeEdges = rp.removeEdges;
    log.edgeRemoveFunction = rp.edgeRemoveFunction;
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
    printEdgeDynamics("", rp.dynamicEdges, rp.edgeAddFunction,
                      rp.removeEdges, rp.edgeRemoveFunction);
}

void simulationStep(od::graph::Graph& graph, std::vector<int>& opinions,
                    const RuntimeParams& rp, std::mt19937& rng, int t,
                    od::io::SimulationLog& log) {
    int stepNumber = t + 1;

    if (rp.dynamicEdges) {
        double addP = od::model::evaluate(rp.edgeAddFunction, t);
        double removeP = rp.removeEdges
            ? od::model::evaluate(rp.edgeRemoveFunction, t)
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

    long long edgesNow = graph.getNumEdges();
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
    std::cout << "Всего рёбер в графе: " << graph.getNumEdges() << "\n";

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
    } else {
        auto [graph, rp] = setupGraphAndParamsInteractive();
        runSimulation(std::move(graph), rp);
    }

    return 0;
}