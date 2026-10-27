#include "od/config/BatchMode.hpp"

#include "od/graph/GraphGenerator.hpp"
#include "od/io/InputHelpers.hpp"
#include "od/model/InitialOpinions.hpp"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace od::config {

namespace {

using od::model::ProbabilityFunction;

std::optional<ProbabilityFunction> buildFunction(const FunctionArgs& args, const std::string& prefix) {
    const std::string funcFlag = "--" + prefix + "-func";
    const std::string paramsFlag = "--" + prefix + "-params";

    if (args.name.empty()) {
        if (!args.params.empty()) {
            throw std::invalid_argument(paramsFlag + " задан без " + funcFlag);
        }
        if (args.legacyA.has_value() || args.legacyB.has_value()) {
            return ProbabilityFunction{od::model::FunctionKind::Linear,
                                       {args.legacyA.value_or(0.0), args.legacyB.value_or(0.0)}};
        }
        return std::nullopt;
    }

    std::optional<od::model::FunctionKind> kind = od::model::kindFromString(args.name);
    if (!kind.has_value()) {
        throw UsageError("неизвестная функция в " + funcFlag + ": " + args.name);
    }

    std::optional<std::vector<double>> params = od::io::parseDoubleList(args.params);
    if (!params.has_value()) {
        throw std::invalid_argument("некорректные числа в " + paramsFlag + ": '" + args.params + "'");
    }

    ProbabilityFunction function{*kind, *params};
    std::string error = od::model::validateFunction(function);
    if (!error.empty()) {
        throw std::invalid_argument(paramsFlag + ": " + error);
    }
    return function;
}

StubbornGroup buildStubbornGroup(const StubbornArgs& args, const std::string& targetsFlag) {
    if (args.manual && args.count > 0 && args.targets.empty()) {
        throw std::invalid_argument("выбран ручной режим, но не задан " + targetsFlag);
    }
    StubbornGroup group;
    group.count = args.count;
    group.manual = args.manual;
    if (group.manual) {
        group.targets = args.targets;
    }
    return group;
}

od::model::OpinionFunction buildOpinionFunction(const BatchArgs& args, od::model::OpinionFunction function) {
    if (!args.opinionFunction.empty()) {
        std::optional<od::model::OpinionFunctionKind> kind = od::model::opinionKindFromString(args.opinionFunction);
        if (!kind.has_value()) {
            throw UsageError("неизвестная функция в --opinion-func: " + args.opinionFunction);
        }
        std::optional<std::vector<double>> params = od::io::parseDoubleList(args.opinionParams);
        if (!params.has_value()) {
            throw std::invalid_argument("некорректные числа в --opinion-params: '" + args.opinionParams + "'");
        }
        function = od::model::OpinionFunction{*kind, *params};
    } else if (!args.opinionParams.empty()) {
        throw std::invalid_argument("--opinion-params задан без --opinion-func");
    }

    if (args.k1.has_value() || args.k2.has_value()) {
        if (!args.opinionFunction.empty() && function.kind != od::model::OpinionFunctionKind::Threshold) {
            throw std::invalid_argument("--k1 и --k2 задают пороговую функцию, а выбрана " + args.opinionFunction);
        }
        bool isThreshold = function.kind == od::model::OpinionFunctionKind::Threshold && function.params.size() == 2;
        function = od::model::thresholdFunction(args.k1.value_or(isThreshold ? function.params[0] : 0.0),
                                                args.k2.value_or(isThreshold ? function.params[1] : 1.0));
    }

    std::string error = od::model::validateOpinionFunction(function);
    if (!error.empty()) {
        throw std::invalid_argument("функция изменения мнений: " + error);
    }
    return function;
}

od::model::ChoiceRule buildChoiceRule(const BatchArgs& args, od::model::ChoiceRule rule) {
    if (args.choiceRule.empty()) {
        return rule;
    }
    std::optional<od::model::ChoiceRule> parsed = od::model::choiceRuleFromString(args.choiceRule);
    if (!parsed.has_value()) {
        throw UsageError("неизвестное правило в --choice-rule: " + args.choiceRule + ", ожидается proportional или max");
    }
    return *parsed;
}

od::model::InitialGroup buildInitialGroup(const std::optional<std::string>& value, const std::string& flag,
                                          od::model::InitialGroup group) {
    if (!value.has_value()) {
        return group;
    }
    std::optional<od::model::InitialGroup> parsed = od::model::initialGroupFromString(*value);
    if (!parsed.has_value()) {
        throw std::invalid_argument("некорректное значение " + flag + ": '" + *value
                                    + "', ожидается random или номер мнения");
    }
    return *parsed;
}

bool hasInitialOverrides(const BatchArgs& args) {
    return args.initialRegular.has_value() || args.initialStubborn.has_value() || args.initialManual.has_value();
}

od::model::InitialOpinionSettings buildInitialOpinions(const BatchArgs& args,
                                                       od::model::InitialOpinionSettings settings) {
    settings.regular = buildInitialGroup(args.initialRegular, "--initial-regular", settings.regular);
    settings.stubborn = buildInitialGroup(args.initialStubborn, "--initial-stubborn", settings.stubborn);
    if (args.initialManual.has_value()) {
        auto manual = od::model::manualOpinionsFromString(*args.initialManual);
        if (!manual.has_value()) {
            throw std::invalid_argument("некорректное значение --initial-opinions: '" + *args.initialManual
                                        + "', ожидается вершина:мнение через запятую");
        }
        settings.manual = *manual;
    }
    return settings;
}

ManualGenConfig buildConfig(const BatchArgs& args) {
    ManualGenConfig config;

    config.cntLevels = static_cast<int>(args.levelVertices.size());
    for (size_t i = 0; i < args.levelVertices.size(); ++i) {
        config.components.push_back(LevelConfig{args.levelVertices[i], args.levelProbabilities[i]});
    }

    config.stubbornAssign = buildStubbornGroup(args.stubbornAssign, "--stubborn-assign-targets");
    config.stubbornAttach = buildStubbornGroup(args.stubbornAttach, "--stubborn-attach-targets");
    config.generateStubborn = config.stubbornAssign.count > 0 || config.stubbornAttach.count > 0;

    config.dynamicEdges = args.dynamicEdges.value_or(false);
    if (config.dynamicEdges) {
        std::optional<ProbabilityFunction> addFunction = buildFunction(args.addFunction, "add");
        if (!addFunction.has_value()) {
            throw std::invalid_argument("--dynamic-edges требует --add-func и --add-params");
        }
        config.edgeAddFunction = *addFunction;
    }

    config.removeEdges = args.removeEdges.value_or(false);
    if (config.removeEdges) {
        std::optional<ProbabilityFunction> removeFunction = buildFunction(args.removeFunction, "remove");
        if (!removeFunction.has_value()) {
            throw std::invalid_argument("--remove-edges требует --remove-func и --remove-params");
        }
        config.edgeRemoveFunction = *removeFunction;
    }

    config.numOpinions = args.numOpinions.value_or(od::model::DEFAULT_NUM_OPINIONS);
    config.opinionFunction = buildOpinionFunction(args, od::model::defaultOpinionFunction());
    config.choiceRule = buildChoiceRule(args, od::model::ChoiceRule::Max);
    config.popularity = args.popularity.value_or(std::vector<int>{});
    config.initialOpinions = buildInitialOpinions(args, od::model::InitialOpinionSettings{});
    config.useWeights = args.useWeights.value_or(false);
    return config;
}

SimulationSetup generate(const BatchArgs& args) {
    ManualGenConfig config = buildConfig(args);

    ValidationResult validation = validateConfig(config);
    if (!validation.isValid) {
        throw std::invalid_argument("некорректная конфигурация: " + validation.errorMessage);
    }

    RuntimeParams params = paramsFromConfig(config);
    params.tMax = args.tMax.value_or(params.tMax);
    if (args.seed.has_value()) {
        params.seed = *args.seed;
        params.seedSpecified = true;
    }
    ensureSeed(params);

    return SimulationSetup{od::graph::generateGraph(config, params.seed), params, {}, {}};
}

void applyOverrides(RuntimeParams& params, const BatchArgs& args) {
    params.numOpinions = args.numOpinions.value_or(params.numOpinions);
    params.opinionFunction = buildOpinionFunction(args, params.opinionFunction);
    params.choiceRule = buildChoiceRule(args, params.choiceRule);
    params.popularity = args.popularity.value_or(params.popularity);
    params.initialOpinions = buildInitialOpinions(args, params.initialOpinions);
    params.tMax = args.tMax.value_or(params.tMax);
    params.useWeights = args.useWeights.value_or(params.useWeights);
    params.dynamicEdges = args.dynamicEdges.value_or(params.dynamicEdges);
    params.removeEdges = args.removeEdges.value_or(params.removeEdges);

    if (args.seed.has_value()) {
        params.seed = *args.seed;
        params.seedSpecified = true;
    }
    if (std::optional<ProbabilityFunction> f = buildFunction(args.addFunction, "add")) {
        params.edgeAddFunction = *f;
    }
    if (std::optional<ProbabilityFunction> f = buildFunction(args.removeFunction, "remove")) {
        params.edgeRemoveFunction = *f;
    }
}

SimulationSetup load(const BatchArgs& args) {
    const std::string filename = args.loadDir + "/"
        + (args.loadFinalGraph ? "graph_final.txt" : "graph_initial.txt");

    od::io::GraphFile file = od::io::loadGraphFile(filename);

    bool opinionsGiven = (args.k1 && args.k2) || !args.opinionFunction.empty();
    if (!file.paramsLoaded && !(opinionsGiven && args.tMax)) {
        throw std::invalid_argument("в " + filename + " нет секции PARAMS, задайте функцию мнений "
                                    "(--k1 и --k2 или --opinion-func) и --tmax");
    }

    RuntimeParams params = file.paramsLoaded ? paramsFromFile(file.params) : RuntimeParams{};
    applyOverrides(params, args);

    std::vector<int> opinions = hasInitialOverrides(args) ? std::vector<int>{} : std::move(file.opinions);
    return SimulationSetup{std::move(file.graph), params, std::move(opinions), {}};
}

} // namespace

SimulationSetup setupBatch(const BatchArgs& args) {
    SimulationSetup setup = args.generate ? generate(args) : load(args);
    setup.output = OutputOptions{args.runsDir, args.quiet};
    return setup;
}

} // namespace od::config
