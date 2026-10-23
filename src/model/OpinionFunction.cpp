#include "od/model/OpinionFunction.hpp"

#include <algorithm>
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

std::string paramCountError(const OpinionFunction& function) {
    const std::string name = opinionKindToString(function.kind);
    const size_t count = function.params.size();
    const std::string received = ", получено " + std::to_string(count);
    const std::string described = " (" + opinionParamsDescription(function.kind) + ")";

    switch (function.kind) {
        case OpinionFunctionKind::Threshold:
        case OpinionFunctionKind::Exponential:
            return count == 2 ? "" : "функции " + name + " нужно параметров: 2" + described + received;
        case OpinionFunctionKind::Moran:
            return count == 1 || count == 2 ? "" : "функции " + name + " нужен 1 или 2 параметра" + described + received;
        case OpinionFunctionKind::Polynomial:
            return count >= 1 ? "" : "функции " + name + " нужен хотя бы один коэффициент";
        case OpinionFunctionKind::Rest:
            return "";
    }
    return "";
}

bool isOpinionNumber(double value) {
    return value >= 0.0 && value < MAX_OPINIONS && value == std::floor(value);
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

std::vector<OpinionChance> adoptionProbabilities(const OpinionRule& rule, const std::vector<OpinionShare>& shares) {
    std::vector<OpinionChance> chances;
    chances.reserve(shares.size() + 1);

    double sum = 0.0;
    for (const OpinionShare& item : shares) {
        if (item.opinion == rule.restOpinion) {
            continue;
        }
        double value = evaluateInfluence(rule.functions[static_cast<size_t>(item.opinion)], item.share);
        chances.push_back(OpinionChance{item.opinion, value});
        sum += value;
    }

    const bool hasRest = rule.restOpinion >= 0;
    if (hasRest && sum <= 1.0) {
        chances.push_back(OpinionChance{rule.restOpinion, 1.0 - sum});
        return chances;
    }

    const bool keepOpinion = sum <= 0.0 || !std::isfinite(sum);
    for (OpinionChance& chance : chances) {
        chance.probability = keepOpinion ? 0.0 : chance.probability / sum;
    }
    if (hasRest) {
        chances.push_back(OpinionChance{rule.restOpinion, 0.0});
    }
    return chances;
}

OpinionFunction thresholdFunction(double k1, double k2) {
    return OpinionFunction{OpinionFunctionKind::Threshold, {k1, k2}};
}

OpinionFunction defaultOpinionFunction() {
    return thresholdFunction(0.0, 1.0);
}

int moranMutant(const OpinionFunction& function) {
    if (function.kind == OpinionFunctionKind::Moran && function.params.size() >= 2) {
        return static_cast<int>(function.params[1]);
    }
    return DEFAULT_MORAN_MUTANT;
}

OpinionRule makeRule(const OpinionFunction& function, int numOpinions, ChoiceRule choice) {
    const size_t size = static_cast<size_t>(numOpinions);
    OpinionRule rule;

    switch (function.kind) {
        case OpinionFunctionKind::Threshold:
            rule.functions.assign(size, function);
            if (choice == ChoiceRule::Proportional) {
                rule.functions[0] = OpinionFunction{OpinionFunctionKind::Rest, {}};
                rule.restOpinion = 0;
            }
            break;
        case OpinionFunctionKind::Moran:
            rule.functions.assign(size, OpinionFunction{OpinionFunctionKind::Moran, {0.0}});
            rule.functions[static_cast<size_t>(moranMutant(function))] =
                OpinionFunction{OpinionFunctionKind::Moran, {function.params[0]}};
            break;
        default:
            rule.functions.assign(size, function);
            break;
    }
    return rule;
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
        case OpinionFunctionKind::Moran:       return "Моран: (1 + delta)*s у мнения-мутанта, s у остальных";
        case OpinionFunctionKind::Polynomial:  return "Полиномиальная: a*s^n + b*s^(n-1) + ... + свободный член";
        case OpinionFunctionKind::Exponential: return "Экспоненциальная: a * e^(b*s)";
        case OpinionFunctionKind::Rest:        return "Остаток: 1 - сумма остальных мнений";
    }
    return "";
}

std::string opinionParamsDescription(OpinionFunctionKind kind) {
    switch (kind) {
        case OpinionFunctionKind::Threshold:   return "k1, k2";
        case OpinionFunctionKind::Moran:       return "delta[, мнение-мутант, по умолчанию 1]";
        case OpinionFunctionKind::Polynomial:  return "коэффициенты от старшей степени к свободному члену";
        case OpinionFunctionKind::Exponential: return "a, b";
        case OpinionFunctionKind::Rest:        return "без параметров";
    }
    return "";
}

std::string choiceRuleToString(ChoiceRule rule) {
    return rule == ChoiceRule::Max ? "max" : "proportional";
}

std::optional<ChoiceRule> choiceRuleFromString(const std::string& name) {
    if (name == "proportional") return ChoiceRule::Proportional;
    if (name == "max") return ChoiceRule::Max;
    return std::nullopt;
}

std::string choiceRuleDisplayName(ChoiceRule rule) {
    return rule == ChoiceRule::Max ? "максимум" : "пропорционально";
}

std::string validateOpinionFunction(const OpinionFunction& function) {
    const std::string name = opinionKindToString(function.kind);
    if (function.kind == OpinionFunctionKind::Rest) {
        return "служебную функцию rest нельзя выбрать для модели";
    }
    std::string countError = paramCountError(function);
    if (!countError.empty()) {
        return countError;
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
    if (function.kind == OpinionFunctionKind::Moran && p.size() == 2 && !isOpinionNumber(p[1])) {
        return "мнение-мутант функции moran должно быть целым числом от 0 до "
               + std::to_string(MAX_OPINIONS - 1);
    }
    return "";
}

std::string validatePopularity(const std::vector<int>& popularity, int numOpinions) {
    std::vector<char> listed(static_cast<size_t>(std::max(numOpinions, 0)), 0);
    for (int opinion : popularity) {
        if (opinion < 0 || opinion >= numOpinions) {
            return "в списке популярности мнение " + std::to_string(opinion) + " вне диапазона от 0 до "
                   + std::to_string(numOpinions - 1);
        }
        if (listed[static_cast<size_t>(opinion)]) {
            return "в списке популярности мнение " + std::to_string(opinion) + " повторяется";
        }
        listed[static_cast<size_t>(opinion)] = 1;
    }
    return "";
}

std::string validateOpinionSettings(const OpinionFunction& function, const std::vector<int>& popularity,
                                    int numOpinions) {
    std::string error = validateOpinionFunction(function);
    if (!error.empty()) {
        return error;
    }
    if (function.kind == OpinionFunctionKind::Moran && moranMutant(function) >= numOpinions) {
        return "мнение-мутант " + std::to_string(moranMutant(function)) + " вне диапазона мнений от 0 до "
               + std::to_string(numOpinions - 1);
    }
    return validatePopularity(popularity, numOpinions);
}

std::string validateOpinionRule(const OpinionRule& rule) {
    const int size = static_cast<int>(rule.functions.size());
    if (size < 2 || size > MAX_OPINIONS) {
        return "в правиле должно быть от 2 до " + std::to_string(MAX_OPINIONS) + " мнений, задано "
               + std::to_string(size);
    }
    for (int k = 0; k < size; ++k) {
        const OpinionFunction& function = rule.functions[static_cast<size_t>(k)];
        if (function.kind == OpinionFunctionKind::Rest) {
            if (k != rule.restOpinion) {
                return "функция rest у мнения " + std::to_string(k) + " не отмечена в restOpinion";
            }
            continue;
        }
        std::string error = validateOpinionFunction(function);
        if (!error.empty()) {
            return "мнение " + std::to_string(k) + ": " + error;
        }
    }
    if (rule.restOpinion >= size
        || (rule.restOpinion >= 0
            && rule.functions[static_cast<size_t>(rule.restOpinion)].kind != OpinionFunctionKind::Rest)) {
        return "restOpinion указывает на мнение без функции rest";
    }
    return "";
}

std::string describeOpinionFunction(const OpinionFunction& function) {
    return opinionKindToString(function.kind) + "(" + formatParams(function.params) + ")";
}

std::string describeOpinionList(const std::vector<int>& opinions) {
    std::ostringstream out;
    for (size_t i = 0; i < opinions.size(); ++i) {
        if (i > 0) out << ", ";
        out << opinions[i];
    }
    return out.str();
}

} // namespace od::model
