#include "od/config/ConfigInput.hpp"

#include "od/config/Limits.hpp"
#include "od/io/InputHelpers.hpp"

#include <cmath>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace od::config {

namespace {

int askLevelCount() {
    return od::io::readIntUntilValid(
        "Введите кол-во уровней от 1 до " + std::to_string(MAX_LEVELS) + ": ", 1, MAX_LEVELS);
}

void collectLevelDetails(ManualGenConfig& config) {
    int enteredLevels = 0;
    int availableVertices = MAX_VERTICES;

    while (enteredLevels != config.cntLevels) {
        const int remainingLevels = config.cntLevels - enteredLevels;
        if (availableVertices < remainingLevels) {
            enteredLevels = 0;
            config.components.clear();
            availableVertices = MAX_VERTICES;
            std::cout << "Не удалось распределить вершины, начинаем ввод уровней заново\n";
            continue;
        }

        std::cout << "Доступное кол-во вершин: " << availableVertices << "\n";

        const std::string prompt = "Введите кол-во вершин для " + std::to_string(enteredLevels + 1) + " уровня: ";
        std::optional<int> vertices = od::io::readIntInRange(prompt, 1, availableVertices);
        if (!vertices.has_value()) continue;

        std::optional<double> probability = od::io::readDoubleInRange("Введите вероятность появления ребра: ", 0.0, 1.0);
        if (!probability.has_value()) continue;

        config.components.push_back(LevelConfig{*vertices, *probability});
        availableVertices -= *vertices;
        ++enteredLevels;
    }
}

void collectLevels(ManualGenConfig& config) {
    config.cntLevels = askLevelCount();
    collectLevelDetails(config);
}

std::vector<int> readStubbornTargets(int count, int levelVertices, bool unique) {
    std::vector<int> targets;
    std::vector<char> used(static_cast<size_t>(levelVertices), 0);
    targets.reserve(static_cast<size_t>(count));

    while (static_cast<int>(targets.size()) < count) {
        const std::string prompt = "Вершина #" + std::to_string(targets.size() + 1)
                                   + " (0.." + std::to_string(levelVertices - 1) + "): ";
        std::optional<int> vertex = od::io::readIntInRange(prompt, 0, levelVertices - 1);
        if (!vertex.has_value()) continue;

        if (unique && used[static_cast<size_t>(*vertex)]) {
            std::cout << "Эта вершина уже выбрана, укажите другую\n";
            continue;
        }
        used[static_cast<size_t>(*vertex)] = 1;
        targets.push_back(*vertex);
    }
    return targets;
}

void collectStubbornGroup(StubbornGroup& group, const std::string& title, int maxCount, int levelVertices,
                          bool unique) {
    group = StubbornGroup{};

    if (maxCount < 1) {
        std::cout << title << ": нет доступных вершин, пропускаем\n";
        return;
    }

    group.count = od::io::readIntUntilValid(
        title + ": сколько вершин (0.." + std::to_string(maxCount) + "): ", 0, maxCount);
    if (group.count == 0) {
        return;
    }

    group.manual = od::io::readYesNo(title + ": выбрать вершины вручную? (y - вручную, n - случайно): ");
    if (group.manual && group.count > MAX_MANUAL_TARGETS) {
        std::cout << "Ручной выбор ограничен " << MAX_MANUAL_TARGETS
                  << " вершинами. Будет использован случайный выбор.\n";
        group.manual = false;
    }
    if (group.manual) {
        group.targets = readStubbornTargets(group.count, levelVertices, unique);
    }
}

void collectStubbornSettings(ManualGenConfig& config) {
    config.stubbornAssign = StubbornGroup{};
    config.stubbornAttach = StubbornGroup{};

    config.generateStubborn = od::io::readYesNo("Нужны ли упрямые вершины. Введите y/n: ");
    if (!config.generateStubborn) {
        return;
    }

    const int levelVertices = countLevelVertices(config);
    collectStubbornGroup(config.stubbornAssign, "Назначить существующие", levelVertices, levelVertices, true);
    collectStubbornGroup(config.stubbornAttach, "Прикрепить новые", maxAttachCount(levelVertices), levelVertices,
                         false);

    if (config.stubbornAssign.count == 0 && config.stubbornAttach.count == 0) {
        std::cout << "Не выбрано ни одной упрямой вершины\n";
        config.generateStubborn = false;
    }
}

od::model::FunctionKind chooseFunctionKind(const std::string& title) {
    const std::vector<od::model::FunctionKind> kinds = od::model::allKinds();

    std::cout << title << ":\n";
    for (size_t i = 0; i < kinds.size(); ++i) {
        std::cout << "  " << (i + 1) << " - " << od::model::kindDisplayName(kinds[i]) << "\n";
    }
    const int choice = od::io::readIntUntilValid("Ввод: ", 1, static_cast<int>(kinds.size()));
    return kinds[static_cast<size_t>(choice - 1)];
}

void printFunctionPreview(const od::model::ProbabilityFunction& function) {
    std::cout << "Проверка: p(0) = " << od::model::evaluate(function, 0)
              << ", p(50) = " << od::model::evaluate(function, 50)
              << ", p(100) = " << od::model::evaluate(function, 100)
              << ", p(500) = " << od::model::evaluate(function, 500) << "\n";
}

od::model::ProbabilityFunction collectProbabilityFunction(const std::string& title) {
    od::model::ProbabilityFunction function;
    function.kind = chooseFunctionKind(title);

    const std::string prompt = "Параметры (" + od::model::paramsDescription(function.kind) + ") через запятую: ";
    while (true) {
        std::optional<std::vector<double>> values = od::io::parseDoubleList(od::io::readLine(prompt));
        if (!values.has_value()) {
            std::cout << "Некорректные числа, повторите ввод\n";
            continue;
        }
        function.params = *values;
        const std::string error = od::model::validateFunction(function);
        if (error.empty()) {
            break;
        }
        std::cout << error << "\n";
    }

    printFunctionPreview(function);
    return function;
}

od::model::OpinionFunction collectThresholds() {
    const std::string k1Prompt = "Введите порог k1 (0.0..1.0): ";
    double k1 = od::io::readDoubleUntilValid(k1Prompt, 0.0, 1.0);
    while (k1 >= 1.0) {
        std::cout << "k1 должен быть меньше 1, иначе нельзя выбрать k2 > k1\n";
        k1 = od::io::readDoubleUntilValid(k1Prompt, 0.0, 1.0);
    }

    const std::string k2Prompt = "Введите порог k2 (больше " + std::to_string(k1) + ", до 1.0): ";
    double k2 = od::io::readDoubleUntilValid(k2Prompt, k1, 1.0);
    while (k2 <= k1) {
        std::cout << "k2 должен быть строго больше k1\n";
        k2 = od::io::readDoubleUntilValid(k2Prompt, k1, 1.0);
    }
    return od::model::thresholdFunction(k1, k2);
}

od::model::OpinionFunctionKind chooseOpinionKind() {
    const std::vector<od::model::OpinionFunctionKind> kinds = od::model::allOpinionKinds();

    std::cout << "Функция изменения мнений (s - доля соседей с мнением):\n";
    for (size_t i = 0; i < kinds.size(); ++i) {
        std::cout << "  " << (i + 1) << " - " << od::model::opinionKindDisplayName(kinds[i]) << "\n";
    }
    const int choice = od::io::readIntUntilValid("Ввод: ", 1, static_cast<int>(kinds.size()));
    return kinds[static_cast<size_t>(choice - 1)];
}

std::string powerName(int power) {
    if (power == 0) return "свободный член";
    if (power == 1) return "коэффициент при s";
    return "коэффициент при s^" + std::to_string(power);
}

od::model::OpinionFunction collectPolynomial() {
    const int degree = od::io::readIntUntilValid(
        "Степень полинома n (0.." + std::to_string(MAX_POLYNOMIAL_DEGREE) + "): ", 0, MAX_POLYNOMIAL_DEGREE);

    od::model::OpinionFunction function{od::model::OpinionFunctionKind::Polynomial, {}};
    for (int power = degree; power >= 0; --power) {
        std::optional<double> value;
        while (!value.has_value() || !std::isfinite(*value)) {
            value = od::io::readDouble("  " + powerName(power) + ": ");
        }
        function.params.push_back(*value);
    }
    return function;
}

od::model::OpinionFunction collectOpinionFunction(int numOpinions) {
    od::model::OpinionFunction function;
    function.kind = chooseOpinionKind();
    if (function.kind == od::model::OpinionFunctionKind::Threshold) {
        return collectThresholds();
    }
    if (function.kind == od::model::OpinionFunctionKind::Polynomial) {
        return collectPolynomial();
    }

    const std::string prompt = "Параметры (" + od::model::opinionParamsDescription(function.kind) + ") через запятую: ";
    while (true) {
        std::optional<std::vector<double>> values = od::io::parseDoubleList(od::io::readLine(prompt));
        if (!values.has_value()) {
            std::cout << "Некорректные числа, повторите ввод\n";
            continue;
        }
        function.params = *values;
        const std::string error = od::model::validateOpinionSettings(function, {}, numOpinions);
        if (error.empty()) {
            return function;
        }
        std::cout << error << "\n";
    }
}

od::model::ChoiceRule chooseChoiceRule() {
    std::cout << "Правило выбора нового мнения:\n"
              << "  1 - максимум: кандидат с наибольшей P, переход в него с вероятностью P\n"
              << "  2 - пропорционально: мнение выбирается случайно с вероятностью P\n";
    return od::io::readIntUntilValid("Ввод: ", 1, 2) == 1
        ? od::model::ChoiceRule::Max
        : od::model::ChoiceRule::Proportional;
}

std::vector<int> collectPopularity(int numOpinions) {
    const std::string prompt = "Популярность мнений через запятую, от самого популярного (0.."
                               + std::to_string(numOpinions - 1) + ", пусто - ничьи случайно): ";
    while (true) {
        std::optional<std::vector<int>> values = od::io::parseIntList(od::io::readLine(prompt));
        if (!values.has_value()) {
            std::cout << "Некорректные числа, повторите ввод\n";
            continue;
        }
        const std::string error = od::model::validatePopularity(*values, numOpinions);
        if (error.empty()) {
            return *values;
        }
        std::cout << error << "\n";
    }
}

od::model::InitialGroup collectInitialGroup(const std::string& title, int numOpinions) {
    std::cout << "Начальные мнения " << title << ":\n"
              << "  1 - одинаковые у всех\n"
              << "  2 - случайные (0.." << (numOpinions - 1) << ")\n";
    if (od::io::readIntUntilValid("Ввод: ", 1, 2) == 2) {
        return od::model::InitialGroup{true, 0};
    }
    const int value = od::io::readIntUntilValid(
        "Мнение (0.." + std::to_string(numOpinions - 1) + "): ", 0, numOpinions - 1);
    return od::model::InitialGroup{false, value};
}

std::vector<od::model::ManualOpinion> collectManualOpinions(int numOpinions) {
    if (!od::io::readYesNo("Задать мнения отдельных вершин вручную? Введите y/n: ")) {
        return {};
    }
    while (true) {
        std::optional<std::vector<od::model::ManualOpinion>> manual = od::model::manualOpinionsFromString(
            od::io::readLine("Пары вершина:мнение через запятую, например 0:2,5:1: "));
        if (!manual.has_value()) {
            std::cout << "Некорректная запись, нужно вершина:мнение через запятую\n";
            continue;
        }
        od::model::InitialOpinionSettings check;
        check.manual = *manual;
        const std::string error = od::model::validateInitialSettings(check, numOpinions);
        if (error.empty()) {
            return *manual;
        }
        std::cout << error << "\n";
    }
}

void collectInitialOpinions(ManualGenConfig& config, bool hasStubborn) {
    config.initialOpinions = od::model::InitialOpinionSettings{};
    config.initialOpinions.regular = collectInitialGroup("обычных вершин", config.numOpinions);
    if (hasStubborn) {
        config.initialOpinions.stubborn = collectInitialGroup("упрямых вершин", config.numOpinions);
    }
    config.initialOpinions.manual = collectManualOpinions(config.numOpinions);
}

void collectGraphRest(ManualGenConfig& config) {
    collectStubbornSettings(config);
    collectDynamicEdgesSettings(config);
    collectOpinionModelSettings(config, config.generateStubborn);
}

} // namespace

GenerationMode chooseGenerationMode() {
    const int choice = od::io::readIntUntilValid(
        "Выберите способ генерации:\n"
        "1 - Ручная (свои параметры для каждого уровня)\n"
        "2 - Общая (одни параметры на все уровни)\n"
        "Ввод: ", 1, 2);
    return choice == 1 ? GenerationMode::Manual : GenerationMode::General;
}

ManualGenConfig collectManualGenConfig() {
    ManualGenConfig config;
    collectLevels(config);
    collectGraphRest(config);
    return config;
}

ManualGenConfig collectGeneralGenConfig() {
    ManualGenConfig config;
    config.cntLevels = askLevelCount();

    const int maxPerLevel = MAX_VERTICES / config.cntLevels;
    const int verticesPerLevel = od::io::readIntUntilValid(
        "Введите кол-во вершин на каждый уровень (1.." + std::to_string(maxPerLevel) + "): ", 1, maxPerLevel);
    const double probability = od::io::readDoubleUntilValid(
        "Введите вероятность появления ребра (0.0..1.0): ", 0.0, 1.0);

    config.components.assign(static_cast<size_t>(config.cntLevels), LevelConfig{verticesPerLevel, probability});
    collectGraphRest(config);
    return config;
}

void collectDynamicEdgesSettings(ManualGenConfig& config) {
    config.dynamicEdges = od::io::readYesNo(
        "Нужна ли динамика рёбер (изменение вероятности связи со временем)? Введите y/n: ");
    config.removeEdges = false;
    if (!config.dynamicEdges) {
        return;
    }

    config.edgeAddFunction = collectProbabilityFunction("Функция вероятности ПОЯВЛЕНИЯ рёбер");
    config.removeEdges = od::io::readYesNo("Нужно ли удаление рёбер? Введите y/n: ");
    if (config.removeEdges) {
        config.edgeRemoveFunction = collectProbabilityFunction("Функция вероятности ИСЧЕЗНОВЕНИЯ рёбер");
    }
}

void collectOpinionModelSettings(ManualGenConfig& config, bool hasStubborn) {
    config.numOpinions = od::io::readIntUntilValid(
        "Число мнений (2.." + std::to_string(od::model::MAX_OPINIONS) + "): ", 2, od::model::MAX_OPINIONS);
    config.opinionFunction = collectOpinionFunction(config.numOpinions);
    config.choiceRule = chooseChoiceRule();
    config.popularity.clear();
    if (config.choiceRule == od::model::ChoiceRule::Max) {
        config.popularity = collectPopularity(config.numOpinions);
    }
    collectInitialOpinions(config, hasStubborn);
    config.useWeights = od::io::readYesNo("Учитывать веса рёбер при изменении мнения? Введите y/n: ");
}

} // namespace od::config
