#include "od/config/RuntimeParams.hpp"

#include <iostream>
#include <random>
#include <stdexcept>

namespace od::config {

namespace {

const int MAX_STEPS = 100000;

void checkFunction(const od::model::ProbabilityFunction& function, const std::string& title) {
    std::string error = od::model::validateFunction(function);
    if (!error.empty()) {
        throw std::invalid_argument(title + ": " + error);
    }
}

} // namespace

RuntimeParams paramsFromConfig(const ManualGenConfig& config) {
    RuntimeParams params;
    params.k1 = config.k1;
    params.k2 = config.k2;
    params.useWeights = config.useWeights;
    params.dynamicEdges = config.dynamicEdges;
    params.edgeAddFunction = config.edgeAddFunction;
    params.removeEdges = config.removeEdges;
    params.edgeRemoveFunction = config.edgeRemoveFunction;
    return params;
}

RuntimeParams paramsFromFile(const od::io::SimulationParams& fileParams) {
    RuntimeParams params;
    params.k1 = fileParams.k1;
    params.k2 = fileParams.k2;
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
    fileParams.k1 = params.k1;
    fileParams.k2 = params.k2;
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

void validateParams(const RuntimeParams& params) {
    if (!(params.k1 >= 0.0 && params.k1 < params.k2 && params.k2 <= 1.0)) {
        throw std::invalid_argument("пороги должны удовлетворять 0 <= k1 < k2 <= 1, получено k1 = "
                                    + std::to_string(params.k1) + ", k2 = " + std::to_string(params.k2));
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
