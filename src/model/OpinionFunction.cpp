#include "od/model/OpinionFunction.hpp"

#include <cmath>
#include <sstream>

namespace od::model {

namespace {

double thresholdValue(double share, double k1, double k2) {
    if (share <= k1) return 0.0;
    if (share <= k2) return (share - k1) / (k2 - k1);
    return 1.0;
}

double polynomialValue(const std::vector<double>& coefficientsFromHighest, double x) {
    double result = 0.0;
    for (double coefficient : coefficientsFromHighest) {
        result = result * x + coefficient;
    }
    return result;
}

double rawValue(const OpinionFunction& f, double s) {
    const std::vector<double>& p = f.params;
    switch (f.kind) {
        case OpinionFunctionKind::Threshold:   return thresholdValue(s, p[0], p[1]);
        case OpinionFunctionKind::Moran:       return (1.0 + p[0]) * s;
        case OpinionFunctionKind::Polynomial:  return polynomialValue(p, s);
        case OpinionFunctionKind::Exponential: return p[0] * std::exp(p[1] * s);
        case OpinionFunctionKind::Rest:        return 0.0;
    }
    return 0.0;
}

int expectedParamCount(OpinionFunctionKind kind) {
    switch (kind) {
        case OpinionFunctionKind::Threshold:   return 2;
        case OpinionFunctionKind::Moran:       return 1;
        case OpinionFunctionKind::Polynomial:  return -1;
        case OpinionFunctionKind::Exponential: return 2;
        case OpinionFunctionKind::Rest:        return 0;
    }
    return 0;
}

int findRest(const OpinionRule& rule) {
    for (size_t k = 0; k < rule.size(); ++k) {
        if (rule[k].kind == OpinionFunctionKind::Rest) {
            return static_cast<int>(k);
        }
    }
    return -1;
}

std::string formatParams(const std::vector<double>& params) {
    std::ostringstream out;
    for (size_t i = 0; i < params.size(); ++i) {
        if (i > 0) out << ", ";
        out << params[i];
    }
    return out.str();
}

} // namespace

double evaluateInfluence(const OpinionFunction& function, double share) {
    double value = rawValue(function, share);
    if (std::isnan(value) || value < 0.0) {
        return 0.0;
    }
    return value;
}

std::vector<double> adoptionProbabilities(const OpinionRule& rule, const std::vector<double>& shares) {
    std::vector<double> probabilities(rule.size(), 0.0);
    double sum = 0.0;
    for (size_t k = 0; k < rule.size(); ++k) {
        probabilities[k] = evaluateInfluence(rule[k], shares[k]);
        sum += probabilities[k];
    }

    int rest = findRest(rule);
    if (rest >= 0 && sum <= 1.0) {
        probabilities[static_cast<size_t>(rest)] = 1.0 - sum;
        return probabilities;
    }

    if (sum <= 0.0 || !std::isfinite(sum)) {
        return std::vector<double>(rule.size(), 0.0);
    }
    for (double& p : probabilities) {
        p /= sum;
    }
    return probabilities;
}

std::vector<int> samplingOrder(const OpinionRule& rule) {
    std::vector<int> order;
    int rest = findRest(rule);
    for (int k = 0; k < static_cast<int>(rule.size()); ++k) {
        if (k != rest) {
            order.push_back(k);
        }
    }
    if (rest >= 0) {
        order.push_back(rest);
    }
    return order;
}

OpinionFunction thresholdFunction(double k1, double k2) {
    return OpinionFunction{OpinionFunctionKind::Threshold, {k1, k2}};
}

OpinionFunction defaultOpinionFunction() {
    return thresholdFunction(0.0, 1.0);
}

OpinionRule makeRule(const OpinionFunction& function) {
    switch (function.kind) {
        case OpinionFunctionKind::Threshold:
            return {OpinionFunction{OpinionFunctionKind::Rest, {}}, function};
        case OpinionFunctionKind::Moran:
            return {OpinionFunction{OpinionFunctionKind::Moran, {0.0}}, function};
        default:
            return OpinionRule(static_cast<size_t>(NUM_OPINIONS), function);
    }
}

std::vector<OpinionFunctionKind> allOpinionKinds() {
    return {
        OpinionFunctionKind::Threshold,
        OpinionFunctionKind::Moran,
        OpinionFunctionKind::Polynomial,
        OpinionFunctionKind::Exponential
    };
}

std::string opinionKindToString(OpinionFunctionKind kind) {
    switch (kind) {
        case OpinionFunctionKind::Threshold:   return "threshold";
        case OpinionFunctionKind::Moran:       return "moran";
        case OpinionFunctionKind::Polynomial:  return "polynomial";
        case OpinionFunctionKind::Exponential: return "exponential";
        case OpinionFunctionKind::Rest:        return "rest";
    }
    return "rest";
}

std::optional<OpinionFunctionKind> opinionKindFromString(const std::string& name) {
    for (OpinionFunctionKind kind : allOpinionKinds()) {
        if (opinionKindToString(kind) == name) {
            return kind;
        }
    }
    return std::nullopt;
}

std::string opinionKindDisplayName(OpinionFunctionKind kind) {
    switch (kind) {
        case OpinionFunctionKind::Threshold:   return "Пороговая: 0 при s <= k1, (s - k1)/(k2 - k1) до k2, 1 после";
        case OpinionFunctionKind::Moran:       return "Моран: (1 + delta)*s";
        case OpinionFunctionKind::Polynomial:  return "Полиномиальная: a*s^n + b*s^(n-1) + ... + свободный член";
        case OpinionFunctionKind::Exponential: return "Экспоненциальная: a * e^(b*s)";
        case OpinionFunctionKind::Rest:        return "Остаток: 1 - сумма остальных мнений";
    }
    return "";
}

std::string opinionParamsDescription(OpinionFunctionKind kind) {
    switch (kind) {
        case OpinionFunctionKind::Threshold:   return "k1, k2";
        case OpinionFunctionKind::Moran:       return "delta";
        case OpinionFunctionKind::Polynomial:  return "коэффициенты от старшей степени к свободному члену";
        case OpinionFunctionKind::Exponential: return "a, b";
        case OpinionFunctionKind::Rest:        return "без параметров";
    }
    return "";
}

std::string validateOpinionFunction(const OpinionFunction& function) {
    const std::string name = opinionKindToString(function.kind);
    if (function.kind == OpinionFunctionKind::Rest) {
        return "служебную функцию rest нельзя выбрать для модели";
    }
    int expected = expectedParamCount(function.kind);
    int actual = static_cast<int>(function.params.size());

    if (expected == -1 && actual < 1) {
        return "функции " + name + " нужен хотя бы один коэффициент";
    }
    if (expected != -1 && actual != expected) {
        return "функции " + name + " нужно параметров: " + std::to_string(expected)
               + " (" + opinionParamsDescription(function.kind) + "), получено " + std::to_string(actual);
    }
    for (double value : function.params) {
        if (!std::isfinite(value)) {
            return "параметры функции " + name + " должны быть конечными числами";
        }
    }

    const std::vector<double>& p = function.params;
    if (function.kind == OpinionFunctionKind::Threshold && !(p[0] >= 0.0 && p[0] < p[1] && p[1] <= 1.0)) {
        return "пороги должны удовлетворять 0 <= k1 < k2 <= 1";
    }
    if (function.kind == OpinionFunctionKind::Moran && p[0] < -1.0) {
        return "у функции moran должно быть delta >= -1";
    }
    return "";
}

std::string validateOpinionRule(const OpinionRule& rule) {
    if (static_cast<int>(rule.size()) != NUM_OPINIONS) {
        return "нужно задать функции для " + std::to_string(NUM_OPINIONS) + " мнений, задано "
               + std::to_string(rule.size());
    }
    int restCount = 0;
    for (size_t k = 0; k < rule.size(); ++k) {
        if (rule[k].kind == OpinionFunctionKind::Rest) {
            ++restCount;
            continue;
        }
        std::string error = validateOpinionFunction(rule[k]);
        if (!error.empty()) {
            return "мнение " + std::to_string(k) + ": " + error;
        }
    }
    if (restCount > 1) {
        return "функция rest может быть только у одного мнения";
    }
    return "";
}

std::string describeOpinionFunction(const OpinionFunction& function) {
    return opinionKindToString(function.kind) + "(" + formatParams(function.params) + ")";
}

} // namespace od::model
