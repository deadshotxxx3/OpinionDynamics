#pragma once

#include <optional>
#include <string>
#include <vector>

namespace od::model {

enum class FunctionKind {
    Constant,
    Linear,
    Polynomial,
    Exponential,
    Saturation,
    Logistic,
    Step,
    Periodic
};

struct ProbabilityFunction {
    FunctionKind kind = FunctionKind::Linear;
    std::vector<double> params = {0.0, 0.0};
};

double evaluate(const ProbabilityFunction& function, int t);

std::vector<FunctionKind> allKinds();
std::string kindToString(FunctionKind kind);
std::optional<FunctionKind> kindFromString(const std::string& name);
std::string kindDisplayName(FunctionKind kind);
std::string paramsDescription(FunctionKind kind);

std::string validateFunction(const ProbabilityFunction& function);

std::string formatParams(const std::vector<double>& params);
std::string describeFunction(const ProbabilityFunction& function);

} // namespace od::model