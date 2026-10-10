#include "od/model/ProbabilityFunction.hpp"

#include <cmath>
#include <sstream>

namespace od::model {

namespace {

double clampProbability(double p) {
    if (std::isnan(p)) return 0.0;
    if (p < 0.0) return 0.0;
    if (p > 1.0) return 1.0;
    return p;
}

double evaluatePolynomial(const std::vector<double>& coefficients, double t) {
    double result = 0.0;
    for (auto it = coefficients.rbegin(); it != coefficients.rend(); ++it) {
        result = result * t + *it;
    }
    return result;
}

double evaluateRaw(const ProbabilityFunction& f, double t) {
    const std::vector<double>& p = f.params;

    switch (f.kind) {
        case FunctionKind::Constant:
            return p[0];
        case FunctionKind::Linear:
            return p[0] + p[1] * t;
        case FunctionKind::Polynomial:
            return evaluatePolynomial(p, t);
        case FunctionKind::Exponential:
            return p[0] * std::exp(p[1] * t);
        case FunctionKind::Saturation:
            return p[0] * (1.0 - std::exp(-p[1] * t));
        case FunctionKind::Logistic:
            return p[0] / (1.0 + std::exp(-p[1] * (t - p[2])));
        case FunctionKind::Step:
            return (t < p[2]) ? p[0] : p[1];
        case FunctionKind::Periodic:
            return p[0] + p[1] * std::sin(p[2] * t);
    }
    return 0.0;
}

int expectedParamCount(FunctionKind kind) {
    switch (kind) {
        case FunctionKind::Constant:    return 1;
        case FunctionKind::Linear:      return 2;
        case FunctionKind::Polynomial:  return -1;
        case FunctionKind::Exponential: return 2;
        case FunctionKind::Saturation:  return 2;
        case FunctionKind::Logistic:    return 3;
        case FunctionKind::Step:        return 3;
        case FunctionKind::Periodic:    return 3;
    }
    return 0;
}

} // namespace

double evaluate(const ProbabilityFunction& function, int t) {
    if (!validateFunction(function).empty()) {
        return 0.0;
    }
    return clampProbability(evaluateRaw(function, static_cast<double>(t)));
}

std::vector<FunctionKind> allKinds() {
    return {
        FunctionKind::Constant,
        FunctionKind::Linear,
        FunctionKind::Polynomial,
        FunctionKind::Exponential,
        FunctionKind::Saturation,
        FunctionKind::Logistic,
        FunctionKind::Step,
        FunctionKind::Periodic
    };
}

std::string kindToString(FunctionKind kind) {
    switch (kind) {
        case FunctionKind::Constant:    return "constant";
        case FunctionKind::Linear:      return "linear";
        case FunctionKind::Polynomial:  return "polynomial";
        case FunctionKind::Exponential: return "exponential";
        case FunctionKind::Saturation:  return "saturation";
        case FunctionKind::Logistic:    return "logistic";
        case FunctionKind::Step:        return "step";
        case FunctionKind::Periodic:    return "periodic";
    }
    return "linear";
}

std::optional<FunctionKind> kindFromString(const std::string& name) {
    for (FunctionKind kind : allKinds()) {
        if (kindToString(kind) == name) {
            return kind;
        }
    }
    return std::nullopt;
}

std::string kindDisplayName(FunctionKind kind) {
    switch (kind) {
        case FunctionKind::Constant:    return "Константная: p = a";
        case FunctionKind::Linear:      return "Линейная: p = a + b*t";
        case FunctionKind::Polynomial:  return "Полиномиальная: p = c0 + c1*t + c2*t^2 + ...";
        case FunctionKind::Exponential: return "Экспоненциальная: p = a * e^(b*t)";
        case FunctionKind::Saturation:  return "Насыщение: p = pmax * (1 - e^(-r*t))";
        case FunctionKind::Logistic:    return "Логистическая: p = pmax / (1 + e^(-r*(t - t0)))";
        case FunctionKind::Step:        return "Ступенчатая: p = a при t < T, иначе b";
        case FunctionKind::Periodic:    return "Периодическая: p = a + b*sin(w*t)";
    }
    return "";
}

std::string paramsDescription(FunctionKind kind) {
    switch (kind) {
        case FunctionKind::Constant:    return "a";
        case FunctionKind::Linear:      return "a, b";
        case FunctionKind::Polynomial:  return "c0, c1, c2, ...";
        case FunctionKind::Exponential: return "a, b";
        case FunctionKind::Saturation:  return "pmax, r";
        case FunctionKind::Logistic:    return "pmax, r, t0";
        case FunctionKind::Step:        return "a, b, T";
        case FunctionKind::Periodic:    return "a, b, w";
    }
    return "";
}

std::string validateFunction(const ProbabilityFunction& function) {
    int expected = expectedParamCount(function.kind);
    int actual = static_cast<int>(function.params.size());

    if (expected == -1) {
        if (actual < 1) {
            return "Полиномиальной функции нужен хотя бы один коэффициент";
        }
    } else if (actual != expected) {
        return "Функции " + kindToString(function.kind) + " нужно параметров: "
               + std::to_string(expected) + " (" + paramsDescription(function.kind)
               + "), получено " + std::to_string(actual);
    }

    for (double value : function.params) {
        if (!std::isfinite(value)) {
            return "Параметры функции должны быть конечными числами";
        }
    }

    return "";
}

std::string formatParams(const std::vector<double>& params) {
    std::ostringstream out;
    for (size_t i = 0; i < params.size(); ++i) {
        if (i > 0) out << ", ";
        out << params[i];
    }
    return out.str();
}

std::string describeFunction(const ProbabilityFunction& function) {
    return kindToString(function.kind) + "(" + formatParams(function.params) + ")";
}

} // namespace od::model