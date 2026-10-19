#include "od/config/BatchMode.hpp"

#include "od/graph/GraphGenerator.hpp"
#include "od/io/InputHelpers.hpp"

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

    config.k1 = args.k1.value_or(0.0);
    config.k2 = args.k2.value_or(1.0);
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

    return SimulationSetup{od::graph::generateGraph(config, params.seed), params};
}

void applyOverrides(RuntimeParams& params, const BatchArgs& args) {
    params.k1 = args.k1.value_or(params.k1);
    params.k2 = args.k2.value_or(params.k2);
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

    od::io::SimulationParams fileParams;
    bool paramsLoaded = false;
    od::graph::Graph graph = od::io::loadGraphWithParams(filename, fileParams, paramsLoaded);

    if (!paramsLoaded && !(args.k1 && args.k2 && args.tMax)) {
        throw std::invalid_argument("в " + filename + " нет секции PARAMS, задайте --k1, --k2 и --tmax");
    }

    RuntimeParams params = paramsLoaded ? paramsFromFile(fileParams) : RuntimeParams{};
    applyOverrides(params, args);

    return SimulationSetup{std::move(graph), params};
}

} // namespace

SimulationSetup setupBatch(const BatchArgs& args) {
    return args.generate ? generate(args) : load(args);
}

} // namespace od::config
