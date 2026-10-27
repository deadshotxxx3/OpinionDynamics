#include "od/config/InteractiveMode.hpp"

#include "od/graph/GraphGenerator.hpp"
#include "od/io/InputHelpers.hpp"

#include <iostream>

namespace od::config {

namespace {

const int MAX_STEPS = 100000;
const int MAX_INTERACTIVE_SEED = 2147483647;

void askStepsAndSeed(RuntimeParams& params) {
    params.tMax = od::io::readIntUntilValid(
        "Введите количество шагов симуляции (1.." + std::to_string(MAX_STEPS) + "): ", 1, MAX_STEPS);

    if (od::io::readYesNo("Задать seed вручную? (y - задать, n - случайный): ")) {
        params.seed = static_cast<unsigned int>(od::io::readIntUntilValid(
            "Введите seed (0.." + std::to_string(MAX_INTERACTIVE_SEED) + "): ", 0, MAX_INTERACTIVE_SEED));
        params.seedSpecified = true;
    }
}

RuntimeParams askAllParams(const od::graph::Graph& graph) {
    bool hasStubborn = false;
    for (int v = 0; v < graph.getNumVertices() && !hasStubborn; ++v) {
        hasStubborn = graph.isStubborn(v);
    }

    ManualGenConfig model;
    collectOpinionModelSettings(model, hasStubborn);
    collectDynamicEdgesSettings(model);

    RuntimeParams params = paramsFromConfig(model);
    askStepsAndSeed(params);
    return params;
}

void printLoadedParams(const RuntimeParams& params) {
    std::cout << "Параметры симуляции восстановлены из файла:\n"
              << "  seed = " << params.seed << "\n"
              << "  T_MAX = " << params.tMax << "\n"
              << "  веса: " << (params.useWeights ? "учитываются" : "не учитываются") << "\n";
    printOpinionModel(params, "  ");
    printEdgeDynamics(params, "  ");
}

SimulationSetup generateNewGraph() {
    ManualGenConfig config = (chooseGenerationMode() == GenerationMode::Manual)
        ? collectManualGenConfig()
        : collectGeneralGenConfig();

    ValidationResult validation = validateConfig(config);
    if (!validation.isValid) {
        throw std::invalid_argument("некорректная конфигурация: " + validation.errorMessage);
    }
    std::cout << "Конфиг корректен.\n\n";

    RuntimeParams params = paramsFromConfig(config);
    askStepsAndSeed(params);
    ensureSeed(params);

    return SimulationSetup{od::graph::generateGraph(config, params.seed), params, {}, {}};
}

SimulationSetup loadSavedGraph() {
    std::string runDir = od::io::readLine("Введите путь к папке прогона (например, runs/run_42): ");

    std::cout << "Какой граф загрузить?\n"
              << "1 - Начальный (graph_initial.txt)\n"
              << "2 - Конечный (graph_final.txt)\n";
    bool initial = od::io::readIntUntilValid("Ввод: ", 1, 2) == 1;
    std::string filename = runDir + "/" + (initial ? "graph_initial.txt" : "graph_final.txt");

    od::io::GraphFile file = od::io::loadGraphFile(filename);
    std::cout << "Граф успешно загружен из " << filename << "\n\n";

    if (!file.paramsLoaded) {
        std::cout << "В файле нет сохранённых параметров - введите их вручную.\n";
        RuntimeParams params = askAllParams(file.graph);
        return SimulationSetup{std::move(file.graph), params, {}, {}};
    }

    RuntimeParams loaded = paramsFromFile(file.params);
    printLoadedParams(loaded);
    if (!file.opinions.empty()) {
        std::cout << "  мнения вершин сохранены в файле и будут взяты оттуда\n";
    }
    if (od::io::readYesNo("Использовать эти параметры? (y - да, n - ввести свои): ")) {
        return SimulationSetup{std::move(file.graph), loaded, std::move(file.opinions), {}};
    }
    RuntimeParams params = askAllParams(file.graph);
    return SimulationSetup{std::move(file.graph), params, {}, {}};
}

} // namespace

SimulationSetup setupInteractive() {
    std::cout << "Выберите способ получения графа:\n"
              << "1 - Сгенерировать новый граф\n"
              << "2 - Загрузить граф из папки прогона\n";
    return od::io::readIntUntilValid("Ввод: ", 1, 2) == 1 ? generateNewGraph() : loadSavedGraph();
}

} // namespace od::config
