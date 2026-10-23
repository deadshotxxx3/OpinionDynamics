#include "od/config/BatchArgs.hpp"

#include "od/io/InputHelpers.hpp"
#include "od/model/OpinionFunction.hpp"
#include "od/model/ProbabilityFunction.hpp"

#include <climits>
#include <cstring>
#include <iostream>
#include <map>

namespace od::config {

namespace {

using SwitchHandler = void (*)(BatchArgs&);
using ValueHandler = void (*)(BatchArgs&, const std::string& flag, const std::string& value);

std::invalid_argument badValue(const std::string& flag, const std::string& value, const std::string& expected) {
    return std::invalid_argument("некорректное значение " + flag + ": '" + value + "', ожидается " + expected);
}

int toInt(const std::string& flag, const std::string& value) {
    std::optional<int> number = od::io::parseInt(value);
    if (!number.has_value()) {
        throw badValue(flag, value, "целое число");
    }
    return *number;
}

double toDouble(const std::string& flag, const std::string& value) {
    std::optional<double> number = od::io::parseDouble(value);
    if (!number.has_value()) {
        throw badValue(flag, value, "число");
    }
    return *number;
}

std::vector<int> toIntList(const std::string& flag, const std::string& value) {
    std::optional<std::vector<int>> list = od::io::parseIntList(value);
    if (!list.has_value()) {
        throw badValue(flag, value, "целые числа через запятую");
    }
    return *list;
}

std::vector<double> toDoubleList(const std::string& flag, const std::string& value) {
    std::optional<std::vector<double>> list = od::io::parseDoubleList(value);
    if (!list.has_value()) {
        throw badValue(flag, value, "числа через запятую");
    }
    return *list;
}

unsigned int toSeed(const std::string& flag, const std::string& value) {
    std::string text = od::io::trim(value);
    try {
        size_t consumed = 0;
        unsigned long long seed = std::stoull(text, &consumed);
        if (!text.empty() && text[0] != '-' && consumed == text.size() && seed <= UINT_MAX) {
            return static_cast<unsigned int>(seed);
        }
    } catch (...) {
    }
    throw badValue(flag, value, "целое от 0 до " + std::to_string(UINT_MAX));
}

bool toManualMode(const std::string& flag, const std::string& value) {
    if (value != "random" && value != "manual") {
        throw badValue(flag, value, "random или manual");
    }
    return value == "manual";
}

bool toLoadFinal(const std::string& flag, const std::string& value) {
    if (value != "initial" && value != "final") {
        throw badValue(flag, value, "initial или final");
    }
    return value == "final";
}

const std::map<std::string, SwitchHandler>& switchFlags() {
    static const std::map<std::string, SwitchHandler> flags = {
        {"--batch",         [](BatchArgs&) {}},
        {"--use-weights",   [](BatchArgs& a) { a.useWeights = true; }},
        {"--no-weights",    [](BatchArgs& a) { a.useWeights = false; }},
        {"--dynamic-edges", [](BatchArgs& a) { a.dynamicEdges = true; }},
        {"--remove-edges",  [](BatchArgs& a) { a.removeEdges = true; }},
    };
    return flags;
}

const std::map<std::string, ValueHandler>& valueFlags() {
    static const ValueHandler attachCount =
        [](BatchArgs& a, const std::string& f, const std::string& v) { a.stubbornAttach.count = toInt(f, v); };
    static const ValueHandler attachMode =
        [](BatchArgs& a, const std::string& f, const std::string& v) { a.stubbornAttach.manual = toManualMode(f, v); };
    static const ValueHandler attachTargets =
        [](BatchArgs& a, const std::string& f, const std::string& v) { a.stubbornAttach.targets = toIntList(f, v); };

    static const std::map<std::string, ValueHandler> flags = {
        {"--level-vertices",      [](BatchArgs& a, const std::string& f, const std::string& v) { a.levelVertices = toIntList(f, v); }},
        {"--level-probabilities", [](BatchArgs& a, const std::string& f, const std::string& v) { a.levelProbabilities = toDoubleList(f, v); }},
        {"--vertices",            [](BatchArgs& a, const std::string& f, const std::string& v) { a.levelVertices = {toInt(f, v)}; }},
        {"--probability",         [](BatchArgs& a, const std::string& f, const std::string& v) { a.levelProbabilities = {toDouble(f, v)}; }},

        {"--load-dir",   [](BatchArgs& a, const std::string&, const std::string& v) { a.loadDir = v; a.generate = false; }},
        {"--load-graph", [](BatchArgs& a, const std::string& f, const std::string& v) { a.loadFinalGraph = toLoadFinal(f, v); }},

        {"--k1",   [](BatchArgs& a, const std::string& f, const std::string& v) { a.k1 = toDouble(f, v); }},
        {"--k2",   [](BatchArgs& a, const std::string& f, const std::string& v) { a.k2 = toDouble(f, v); }},
        {"--tmax", [](BatchArgs& a, const std::string& f, const std::string& v) { a.tMax = toInt(f, v); }},
        {"--opinion-func",   [](BatchArgs& a, const std::string&, const std::string& v) { a.opinionFunction = v; }},
        {"--opinion-params", [](BatchArgs& a, const std::string&, const std::string& v) { a.opinionParams = v; }},
        {"--num-opinions",   [](BatchArgs& a, const std::string& f, const std::string& v) { a.numOpinions = toInt(f, v); }},
        {"--choice-rule",    [](BatchArgs& a, const std::string&, const std::string& v) { a.choiceRule = v; }},
        {"--initial-regular",  [](BatchArgs& a, const std::string&, const std::string& v) { a.initialRegular = v; }},
        {"--initial-stubborn", [](BatchArgs& a, const std::string&, const std::string& v) { a.initialStubborn = v; }},
        {"--initial-opinions", [](BatchArgs& a, const std::string&, const std::string& v) { a.initialManual = v; }},
        {"--popularity",     [](BatchArgs& a, const std::string& f, const std::string& v) { a.popularity = toIntList(f, v); }},
        {"--seed", [](BatchArgs& a, const std::string& f, const std::string& v) { a.seed = toSeed(f, v); }},

        {"--stubborn-attach",         attachCount},
        {"--stubborn",                attachCount},
        {"--stubborn-attach-mode",    attachMode},
        {"--stubborn-mode",           attachMode},
        {"--stubborn-attach-targets", attachTargets},
        {"--stubborn-targets",        attachTargets},
        {"--stubborn-assign",         [](BatchArgs& a, const std::string& f, const std::string& v) { a.stubbornAssign.count = toInt(f, v); }},
        {"--stubborn-assign-mode",    [](BatchArgs& a, const std::string& f, const std::string& v) { a.stubbornAssign.manual = toManualMode(f, v); }},
        {"--stubborn-assign-targets", [](BatchArgs& a, const std::string& f, const std::string& v) { a.stubbornAssign.targets = toIntList(f, v); }},

        {"--add-func",      [](BatchArgs& a, const std::string&, const std::string& v) { a.addFunction.name = v; }},
        {"--add-params",    [](BatchArgs& a, const std::string&, const std::string& v) { a.addFunction.params = v; }},
        {"--p0",            [](BatchArgs& a, const std::string& f, const std::string& v) { a.addFunction.legacyA = toDouble(f, v); }},
        {"--k",             [](BatchArgs& a, const std::string& f, const std::string& v) { a.addFunction.legacyB = toDouble(f, v); }},
        {"--remove-func",   [](BatchArgs& a, const std::string&, const std::string& v) { a.removeFunction.name = v; }},
        {"--remove-params", [](BatchArgs& a, const std::string&, const std::string& v) { a.removeFunction.params = v; }},
        {"--remove-p0",     [](BatchArgs& a, const std::string& f, const std::string& v) { a.removeFunction.legacyA = toDouble(f, v); }},
        {"--remove-k",      [](BatchArgs& a, const std::string& f, const std::string& v) { a.removeFunction.legacyB = toDouble(f, v); }},
    };
    return flags;
}

void checkLevels(const BatchArgs& args) {
    if (args.levelVertices.empty() || args.levelProbabilities.empty()) {
        throw UsageError("не заданы уровни графа: нужны --level-vertices и --level-probabilities "
                         "или --vertices и --probability");
    }
    if (args.levelVertices.size() != args.levelProbabilities.size()) {
        throw std::invalid_argument("в --level-vertices " + std::to_string(args.levelVertices.size())
                                    + " уровней, а в --level-probabilities "
                                    + std::to_string(args.levelProbabilities.size()));
    }
}

} // namespace

bool hasBatchFlag(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--batch") == 0) {
            return true;
        }
    }
    return false;
}

BatchArgs parseBatchArgs(int argc, char** argv) {
    BatchArgs args;

    for (int i = 1; i < argc; ++i) {
        const std::string flag = argv[i];

        auto switchFlag = switchFlags().find(flag);
        if (switchFlag != switchFlags().end()) {
            switchFlag->second(args);
            continue;
        }

        auto valueFlag = valueFlags().find(flag);
        if (valueFlag == valueFlags().end()) {
            throw UsageError("неизвестный флаг " + flag);
        }
        if (i + 1 >= argc) {
            throw UsageError("флагу " + flag + " не хватает значения");
        }
        valueFlag->second(args, flag, argv[++i]);
    }

    if (args.generate) {
        checkLevels(args);
    }
    return args;
}

void printBatchUsage() {
    std::cout << "Использование batch-режима:\n\n"
              << "Генерация графа:\n"
              << "  --batch --level-vertices \"10,20,15\" --level-probabilities \"0.3,0.5,0.4\"\n"
              << "  или (один уровень):\n"
              << "  --batch --vertices 100 --probability 0.3\n\n"
              << "Загрузка из папки прогона:\n"
              << "  --batch --load-dir runs/run_42 [--load-graph initial|final]\n\n"
              << "Параметры симуляции (при загрузке без PARAMS функция мнений и --tmax обязательны):\n"
              << "  [--tmax N] [--seed N]\n\n"
              << "Функция изменения мнений (по умолчанию пороговая с k1 = 0, k2 = 1):\n"
              << "  --opinion-func NAME --opinion-params \"a,b,...\"\n"
              << "  или для пороговой коротко: --k1 X --k2 Y\n"
              << "  например: --opinion-func moran --opinion-params 0.5 (второй параметр - мнение-мутант, по умолчанию 1)\n"
              << "  у polynomial коэффициенты от старшей степени: \"3,2,1\" = 3s^2 + 2s + 1\n\n"
              << "Число мнений (по умолчанию 2, мнения от 0 до N-1):\n"
              << "  --num-opinions N (от 2 до " << od::model::MAX_OPINIONS << ")\n\n"
              << "Правило выбора нового мнения (по умолчанию max):\n"
              << "  --choice-rule max|proportional [--popularity \"5,3,2\"]\n"
              << "  max - кандидат с наибольшей P, переход в него с вероятностью P;\n"
              << "  proportional - мнение выбирается случайно с вероятностью P (как в старых прогонах);\n"
              << "  --popularity решает ничьи в max (от самого популярного), без него ничьи случайны\n\n"
              << "Начальные мнения (по умолчанию обычные 0, упрямые 1):\n"
              << "  --initial-regular random|K --initial-stubborn random|K\n"
              << "  --initial-opinions \"0:2,5:1\" (вершина:мнение, перекрывает группы)\n"
              << "  при загрузке без этих флагов мнения берутся из файла графа, если они там есть\n\n"
              << "Опции:\n"
              << "  --use-weights | --no-weights\n"
              << "  --stubborn-attach N [--stubborn-attach-mode random|manual] [--stubborn-attach-targets 0,2,4]\n"
              << "  --stubborn-assign N [--stubborn-assign-mode random|manual] [--stubborn-assign-targets 1,3,5]\n"
              << "  --dynamic-edges --add-func NAME --add-params \"a,b,...\"\n"
              << "  --remove-edges --remove-func NAME --remove-params \"a,b,...\"\n"
              << "  (устаревшее: --p0 X --k Y и --remove-p0 X --remove-k Y задают линейную функцию)\n\n"
              << "Функции вероятности рёбер:\n";
    for (od::model::FunctionKind kind : od::model::allKinds()) {
        std::cout << "  " << od::model::kindToString(kind) << " (" << od::model::paramsDescription(kind)
                  << ") - " << od::model::kindDisplayName(kind) << "\n";
    }
    std::cout << "\nФункции изменения мнений (s - доля соседей с мнением):\n";
    for (od::model::OpinionFunctionKind kind : od::model::allOpinionKinds()) {
        std::cout << "  " << od::model::opinionKindToString(kind) << " (" << od::model::opinionParamsDescription(kind)
                  << ") - " << od::model::opinionKindDisplayName(kind) << "\n";
    }
}

} // namespace od::config
