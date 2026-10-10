#include "od/config/ManualGenConfig.hpp"
#include "od/io/InputHelpers.hpp"

#include <algorithm>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

const int MAX_VERTICES = 20000;
const int MAX_LEVELS = 10000;
const int MAX_MANUAL_ATTACH = 100;

namespace od::config {

static int countTotalVertices(const ManualGenConfig& cnf){
    int total = 0;
    for (const auto& level : cnf.components) total += level.numVertices;
    return total;
}

static int maxAttachCount(int totalVertices){
    return std::min(totalVertices, MAX_VERTICES - totalVertices);
}

static void collectLevelDetails(ManualGenConfig& cnf){
    int enteredLevels = 0;
    int availableVertices = MAX_VERTICES;

    while (enteredLevels != cnf.cntLevels){
        int remainingLevels = cnf.cntLevels - enteredLevels;
        if (availableVertices < remainingLevels){
            enteredLevels = 0;
            cnf.components.clear();
            availableVertices = MAX_VERTICES;
            std::cout << "Не удалось распределить вершины, начинаем ввод уровней заново\n";
            continue;
        }

        std::cout << "Доступное кол-во вершин: " << availableVertices << "\n";

        std::string prompt = "Введите кол-во вершин для " + std::to_string(enteredLevels + 1) + " уровня: ";
        auto vertices = od::io::readIntInRange(prompt, 1, availableVertices);
        if (!vertices.has_value()) continue;

        auto probability = od::io::readDoubleInRange("Введите вероятность появления ребра: ", 0.0, 1.0);
        if (!probability.has_value()) continue;

        cnf.components.push_back(LevelConfig{*vertices, *probability});
        availableVertices -= *vertices;
        ++enteredLevels;
    }
}

void collectLevels(ManualGenConfig& cnf){
    while (true){
        auto val = od::io::readIntInRange(
            "Введите кол-во уровней от 1 до " + std::to_string(MAX_LEVELS) + ": ", 1, MAX_LEVELS);
        if (!val.has_value()) continue;
        cnf.cntLevels = *val;
        break;
    }

    collectLevelDetails(cnf);
}

static std::vector<int> readStubbornTargets(int count, int totalVertices, bool unique){
    std::vector<int> targets;
    std::vector<char> used(static_cast<size_t>(totalVertices), 0);
    targets.reserve(static_cast<size_t>(count));

    while (static_cast<int>(targets.size()) < count) {
        std::string prompt = "Вершина #" + std::to_string(targets.size() + 1)
                             + " (0.." + std::to_string(totalVertices - 1) + "): ";
        auto val = od::io::readIntInRange(prompt, 0, totalVertices - 1);
        if (!val.has_value()) continue;

        if (unique && used[static_cast<size_t>(*val)]) {
            std::cout << "Эта вершина уже выбрана, укажите другую\n";
            continue;
        }
        used[static_cast<size_t>(*val)] = 1;
        targets.push_back(*val);
    }
    return targets;
}

static void collectStubbornGroup(StubbornGroup& group, const std::string& title,
                                 int maxCount, int totalVertices, bool unique){
    group = StubbornGroup{};

    if (maxCount < 1){
        std::cout << title << ": нет доступных вершин, пропускаем\n";
        return;
    }

    while (true) {
        auto val = od::io::readIntInRange(
            title + ": сколько вершин (0.." + std::to_string(maxCount) + "): ", 0, maxCount);
        if (!val.has_value()) continue;
        group.count = *val;
        break;
    }

    if (group.count == 0){
        return;
    }

    group.manual = od::io::readYesNo(
        title + ": выбрать вершины вручную? (y - вручную, n - случайно): ");

    if (group.manual && group.count > MAX_MANUAL_ATTACH){
        std::cout << "Ручной выбор ограничен " << MAX_MANUAL_ATTACH
                  << " вершинами. Будет использован случайный выбор.\n";
        group.manual = false;
    }

    if (group.manual){
        group.targets = readStubbornTargets(group.count, totalVertices, unique);
    }
}

void collectStubbornSettings(ManualGenConfig& cnf){
    cnf.stubbornAssign = StubbornGroup{};
    cnf.stubbornAttach = StubbornGroup{};

    cnf.generateStubborn = od::io::readYesNo("Нужны ли упрямые вершины. Введите y/n: ");
    if (!cnf.generateStubborn){
        return;
    }

    int totalVertices = countTotalVertices(cnf);

    collectStubbornGroup(cnf.stubbornAssign, "Назначить существующие",
                         totalVertices, totalVertices, true);
    collectStubbornGroup(cnf.stubbornAttach, "Прикрепить новые",
                         maxAttachCount(totalVertices), totalVertices, false);

    if (cnf.stubbornAssign.count == 0 && cnf.stubbornAttach.count == 0){
        std::cout << "Не выбрано ни одной упрямой вершины\n";
        cnf.generateStubborn = false;
    }
}

static double readDoubleLoop(const std::string& prompt, double minVal, double maxVal){
    while (true) {
        auto val = od::io::readDoubleInRange(prompt, minVal, maxVal);
        if (val.has_value()) return *val;
    }
}

static std::optional<std::vector<double>> parseDoubleCsv(const std::string& line){
    std::vector<double> values;
    std::stringstream ss(line);
    std::string item;

    while (std::getline(ss, item, ',')) {
        item = od::io::trim(item);
        if (item.empty()) continue;
        try {
            size_t pos = 0;
            double value = std::stod(item, &pos);
            if (pos != item.size()) return std::nullopt;
            values.push_back(value);
        } catch (...) {
            return std::nullopt;
        }
    }
    return values;
}

static od::model::FunctionKind chooseFunctionKind(const std::string& title){
    std::vector<od::model::FunctionKind> kinds = od::model::allKinds();

    std::cout << title << ":\n";
    for (size_t i = 0; i < kinds.size(); ++i) {
        std::cout << "  " << (i + 1) << " - " << od::model::kindDisplayName(kinds[i]) << "\n";
    }

    while (true) {
        auto val = od::io::readIntInRange("Ввод: ", 1, static_cast<int>(kinds.size()));
        if (val.has_value()) return kinds[static_cast<size_t>(*val - 1)];
    }
}

static void printFunctionPreview(const od::model::ProbabilityFunction& function){
    std::cout << "Проверка: p(0) = " << od::model::evaluate(function, 0)
              << ", p(50) = " << od::model::evaluate(function, 50)
              << ", p(100) = " << od::model::evaluate(function, 100)
              << ", p(500) = " << od::model::evaluate(function, 500) << "\n";
}

od::model::ProbabilityFunction collectProbabilityFunction(const std::string& title){
    od::model::ProbabilityFunction function;
    function.kind = chooseFunctionKind(title);

    std::string prompt = "Параметры (" + od::model::paramsDescription(function.kind) + ") через запятую: ";

    while (true) {
        std::cout << prompt;
        std::string line;
        std::getline(std::cin, line);

        auto values = parseDoubleCsv(line);
        if (!values.has_value()) {
            std::cout << "Некорректные числа, повторите ввод\n";
            continue;
        }

        function.params = *values;
        std::string error = od::model::validateFunction(function);
        if (!error.empty()) {
            std::cout << error << "\n";
            continue;
        }
        break;
    }

    printFunctionPreview(function);
    return function;
}

void collectDynamicEdgesSettings(ManualGenConfig& cnf){
    cnf.dynamicEdges = od::io::readYesNo(
        "Нужна ли динамика рёбер (изменение вероятности связи со временем)? Введите y/n: ");
    cnf.removeEdges = false;

    if (!cnf.dynamicEdges){
        return;
    }

    cnf.edgeAddFunction = collectProbabilityFunction("Функция вероятности ПОЯВЛЕНИЯ рёбер");

    cnf.removeEdges = od::io::readYesNo("Нужно ли удаление рёбер? Введите y/n: ");
    if (cnf.removeEdges) {
        cnf.edgeRemoveFunction = collectProbabilityFunction("Функция вероятности ИСЧЕЗНОВЕНИЯ рёбер");
    }
}

void collectOpinionModelSettings(ManualGenConfig& cnf){
    cnf.k1 = readDoubleLoop("Введите порог k1 (0.0..1.0): ", 0.0, 1.0);
    cnf.k2 = readDoubleLoop("Введите порог k2 (" + std::to_string(cnf.k1) + "..1.0): ", cnf.k1, 1.0);
    cnf.useWeights = od::io::readYesNo(
        "Учитывать веса рёбер при изменении мнения? Введите y/n: ");
}

GenerationMode chooseGenerationMode(){
    while (true) {
        auto val = od::io::readIntInRange(
            "Выберите способ генерации:\n"
            "1 - Ручная (свои параметры для каждого уровня)\n"
            "2 - Общая (одни параметры на все уровни)\n"
            "Ввод: ", 1, 2);
        if (!val.has_value()) continue;
        return (*val == 1) ? GenerationMode::Manual : GenerationMode::General;
    }
}

ManualGenConfig collectGeneralGenConfig(){
    ManualGenConfig cnf;

    while (true) {
        auto val = od::io::readIntInRange(
            "Введите кол-во уровней от 1 до " + std::to_string(MAX_LEVELS) + ": ", 1, MAX_LEVELS);
        if (!val.has_value()) continue;
        cnf.cntLevels = *val;
        break;
    }

    int maxPerLevel = MAX_VERTICES / cnf.cntLevels;
    int verticesPerLevel = 0;
    while (true) {
        auto val = od::io::readIntInRange(
            "Введите кол-во вершин на каждый уровень (1.." + std::to_string(maxPerLevel) + "): ",
            1, maxPerLevel);
        if (!val.has_value()) continue;
        verticesPerLevel = *val;
        break;
    }

    double probability = readDoubleLoop("Введите вероятность появления ребра (0.0..1.0): ", 0.0, 1.0);

    cnf.components.assign(static_cast<size_t>(cnf.cntLevels), LevelConfig{verticesPerLevel, probability});

    collectStubbornSettings(cnf);
    collectDynamicEdgesSettings(cnf);
    collectOpinionModelSettings(cnf);

    return cnf;
}

ManualGenConfig collectManualGenConfig(){
    ManualGenConfig cnf;
    collectLevels(cnf);
    collectStubbornSettings(cnf);
    collectDynamicEdgesSettings(cnf);
    collectOpinionModelSettings(cnf);
    return cnf;
}

static ValidationResult validateLevels(const ManualGenConfig& config){
    if (config.cntLevels < 1){
        return {false, "Количество уровней должно быть не меньше 1"};
    }

    if (static_cast<int>(config.components.size()) != config.cntLevels){
        return {false, "Количество введённых уровней не совпадает с cntLevels"};
    }

    for (const auto& level : config.components){
        if (level.numVertices <= 0){
            return {false, "Количество вершин в уровне должно быть больше 0"};
        }
        if (level.probability < 0.0 || level.probability > 1.0){
            return {false, "Вероятность уровня должна быть в диапазоне [0.0, 1.0]"};
        }
    }

    if (countTotalVertices(config) > MAX_VERTICES){
        return {false, "Суммарное количество вершин превышает " + std::to_string(MAX_VERTICES)};
    }

    return {true, ""};
}

static ValidationResult validateStubbornGroup(const StubbornGroup& group, const std::string& title,
                                              int maxCount, int totalVertices, bool unique){
    if (group.count < 0 || group.count > maxCount){
        return {false, title + ": количество должно быть от 0 до " + std::to_string(maxCount)};
    }

    if (!group.manual || group.count == 0){
        return {true, ""};
    }

    if (static_cast<int>(group.targets.size()) != group.count){
        return {false, title + ": число выбранных вершин не совпадает с количеством"};
    }

    std::vector<char> used(static_cast<size_t>(totalVertices), 0);
    for (int idx : group.targets){
        if (idx < 0 || idx >= totalVertices){
            return {false, title + ": индекс вершины вне диапазона"};
        }
        if (unique){
            if (used[static_cast<size_t>(idx)]){
                return {false, title + ": вершина выбрана дважды"};
            }
            used[static_cast<size_t>(idx)] = 1;
        }
    }
    return {true, ""};
}

static ValidationResult validateStubborn(const ManualGenConfig& config){
    if (!config.generateStubborn){
        return {true, ""};
    }

    int totalVertices = countTotalVertices(config);

    ValidationResult assignResult = validateStubbornGroup(
        config.stubbornAssign, "Назначение существующих",
        totalVertices, totalVertices, true);
    if (!assignResult.isValid) return assignResult;

    ValidationResult attachResult = validateStubbornGroup(
        config.stubbornAttach, "Прикрепление новых",
        maxAttachCount(totalVertices), totalVertices, false);
    if (!attachResult.isValid) return attachResult;

    if (config.stubbornAssign.count == 0 && config.stubbornAttach.count == 0){
        return {false, "Включены упрямые вершины, но не выбрано ни одной"};
    }

    return {true, ""};
}

static ValidationResult validateDynamicEdges(const ManualGenConfig& config){
    if (!config.dynamicEdges){
        return {true, ""};
    }

    std::string addError = od::model::validateFunction(config.edgeAddFunction);
    if (!addError.empty()){
        return {false, "Функция появления рёбер: " + addError};
    }

    if (config.removeEdges){
        std::string removeError = od::model::validateFunction(config.edgeRemoveFunction);
        if (!removeError.empty()){
            return {false, "Функция исчезновения рёбер: " + removeError};
        }
    }
    return {true, ""};
}

static ValidationResult validateOpinionModel(const ManualGenConfig& config){
    if (config.k1 < 0.0 || config.k1 > 1.0 || config.k2 < 0.0 || config.k2 > 1.0) {
        return {false, "k1 и k2 должны быть в диапазоне [0.0, 1.0]"};
    }
    if (config.k1 >= config.k2) {
        return {false, "k1 должен быть меньше k2"};
    }
    return {true, ""};
}

ValidationResult validateConfig(const ManualGenConfig& config){
    ValidationResult result = validateLevels(config);
    if (!result.isValid) return result;

    result = validateStubborn(config);
    if (!result.isValid) return result;

    result = validateDynamicEdges(config);
    if (!result.isValid) return result;

    return validateOpinionModel(config);
}

} // namespace od::config