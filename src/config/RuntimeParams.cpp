#include "od/config/RuntimeParams.hpp"

#include "od/config/Limits.hpp"

#include <iostream>
#include <random>
#include <stdexcept>

namespace od::config {

namespace {

void checkFunction(const od::model::ProbabilityFunction& function, const std::string& title) {
    std::string error = od::model::validateFunction(function);
    if (!error.empty()) {
        throw std::invalid_argument(title + ": " + error);
    }
}

} // namespace

RuntimeParams paramsFromConfig(const ManualGenConfig& config) {
    RuntimeParams params;
    params.numOpinions = config.numOpinions;
    params.opinionFunction = config.opinionFunction;
    params.choiceRule = config.choiceRule;
    params.popularity = config.popularity;
    params.initialOpinions = config.initialOpinions;
    params.useWeights = config.useWeights;
    params.dynamicEdges = config.dynamicEdges;
    params.edgeAddFunction = config.edgeAddFunction;
    params.removeEdges = config.removeEdges;
    params.edgeRemoveFunction = config.edgeRemoveFunction;
    return params;
}

RuntimeParams paramsFromFile(const od::io::SimulationParams& fileParams) {
    RuntimeParams params;
    params.numOpinions = fileParams.numOpinions;
    params.opinionFunction = fileParams.opinionFunction;
    params.choiceRule = fileParams.choiceRule;
    params.popularity = fileParams.popularity;
    params.initialOpinions = fileParams.initialOpinions;
    params.useWeights = fileParams.useWeights;
    params.dynamicEdges = fileParams.dynamicEdges;
    params.edgeAddFunction = fileParams.edgeAddFunction;
    params.removeEdges = fileParams.removeEdges;
    params.edgeRemoveFunction = fileParams.edgeRemoveFunction;
    params.tMax = fileParams.tMax;
    params.seed = fileParams.seed;
    params.seedSpecified = true;
    return params;
}

od::io::SimulationParams toFileParams(const RuntimeParams& params) {
    od::io::SimulationParams fileParams;
    fileParams.seed = params.seed;
    fileParams.tMax = params.tMax;
    fileParams.numOpinions = params.numOpinions;
    fileParams.opinionFunction = params.opinionFunction;
    fileParams.choiceRule = params.choiceRule;
    fileParams.popularity = params.popularity;
    fileParams.initialOpinions = params.initialOpinions;
    fileParams.useWeights = params.useWeights;
    fileParams.dynamicEdges = params.dynamicEdges;
    fileParams.edgeAddFunction = params.edgeAddFunction;
    fileParams.removeEdges = params.removeEdges;
    fileParams.edgeRemoveFunction = params.edgeRemoveFunction;
    return fileParams;
}

void ensureSeed(RuntimeParams& params) {
    if (!params.seedSpecified) {
        params.seed = std::random_device{}();
        params.seedSpecified = true;
    }
}

std::string validateNumOpinions(int numOpinions) {
    if (numOpinions < 2 || numOpinions > od::model::MAX_OPINIONS) {
        return "число мнений должно быть от 2 до " + std::to_string(od::model::MAX_OPINIONS)
               + ", получено " + std::to_string(numOpinions);
    }
    return "";
}

void validateParams(const RuntimeParams& params) {
    std::string opinionError = validateNumOpinions(params.numOpinions);
    if (opinionError.empty()) {
        opinionError = od::model::validateOpinionSettings(params.opinionFunction, params.popularity, params.numOpinions);
    }
    if (opinionError.empty()) {
        opinionError = od::model::validateInitialSettings(params.initialOpinions, params.numOpinions);
    }
    if (!opinionError.empty()) {
        throw std::invalid_argument("модель мнений: " + opinionError);
    }
    if (params.tMax < 1 || params.tMax > MAX_STEPS) {
        throw std::invalid_argument("число шагов должно быть от 1 до " + std::to_string(MAX_STEPS)
                                    + ", получено " + std::to_string(params.tMax));
    }
    if (params.dynamicEdges) {
        checkFunction(params.edgeAddFunction, "функция появления рёбер");
    }
    if (params.dynamicEdges && params.removeEdges) {
        checkFunction(params.edgeRemoveFunction, "функция исчезновения рёбер");
    }
}

void printOpinionModel(const RuntimeParams& params, const std::string& indent) {
    std::cout << indent << "Число мнений: " << params.numOpinions << "\n";
    std::cout << indent << "Функция изменения мнений: "
              << od::model::describeOpinionFunction(params.opinionFunction) << "\n";
    std::cout << indent << "Правило выбора: " << od::model::choiceRuleDisplayName(params.choiceRule);
    if (params.choiceRule == od::model::ChoiceRule::Max) {
        std::cout << (params.popularity.empty()
                          ? ", ничьи - случайно"
                          : ", ничьи - по популярности " + od::model::describeOpinionList(params.popularity));
    }
    std::cout << "\n";

    const od::model::InitialOpinionSettings& initial = params.initialOpinions;
    std::cout << indent << "Начальные мнения: обычные - " << od::model::describeInitialGroup(initial.regular)
              << ", упрямые - " << od::model::describeInitialGroup(initial.stubborn);
    if (!initial.manual.empty()) {
        std::cout << ", вручную " << od::model::manualOpinionsToString(initial.manual);
    }
    std::cout << "\n";
}

void printEdgeDynamics(const RuntimeParams& params, const std::string& indent) {
    if (!params.dynamicEdges) {
        std::cout << indent << "Динамика рёбер: выключена\n";
        return;
    }
    std::cout << indent << "Появление рёбер: " << od::model::describeFunction(params.edgeAddFunction) << "\n";
    if (params.removeEdges) {
        std::cout << indent << "Исчезновение рёбер: "
                  << od::model::describeFunction(params.edgeRemoveFunction) << "\n";
    } else {
        std::cout << indent << "Исчезновение рёбер: выключено\n";
    }
}

} // namespace od::config
