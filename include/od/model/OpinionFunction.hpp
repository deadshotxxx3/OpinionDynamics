#pragma once

#include <optional>
#include <string>
#include <vector>

namespace od::model {

/// @brief Вид функции влияния мнения.
enum class OpinionFunctionKind {
    Threshold,   ///< Пороговая: 0 при s ≤ k1, линейно до 1 на (k1, k2], 1 при s > k2.
    Moran,       ///< Процесс Морана: (1 + δ)·s.
    Polynomial,  ///< Полиномиальная: a·sⁿ + b·sⁿ⁻¹ + … + свободный член, коэффициенты от старшей степени.
                 ///< Линейная функция a + b·s — это полином первой степени с коэффициентами `b, a`.
    Exponential, ///< Экспоненциальная: a·e^(b·s).
    Rest         ///< Служебная: вся вероятность, не занятая другими мнениями. Пользователь её не выбирает.
};

/**
 * @brief Функция изменения мнений.
 *
 * Аргумент функции — доля `s` соседей вершины, у которых данное мнение (с весами рёбер или без).
 * Значение — «сила притяжения» мнения: чем оно больше, тем вероятнее вершина его примет.
 * Отрицательные значения и `NaN` считаются нулём.
 *
 * Сейчас в модели одна функция на все мнения, её превращает в правило `makeRule`.
 */
struct OpinionFunction {
    OpinionFunctionKind kind = OpinionFunctionKind::Threshold;
    std::vector<double> params = {0.0, 1.0};
};

/**
 * @brief Правило смены мнений: функция влияния для каждого мнения, индекс — номер мнения.
 *
 * Это внутреннее представление, рассчитанное на будущее расширение, когда у каждого мнения
 * будет своя функция. Сейчас правило строится из одной функции через `makeRule`.
 *
 * Вершина выбирает новое мнение `k` с вероятностью, пропорциональной `g_k(s_k)`:
 * `P(k) = g_k / Σ g`. Если сумма равна нулю, вершина сохраняет мнение.
 *
 * Если у одного из мнений функция `Rest`, значения остальных функций считаются
 * вероятностями напрямую, а мнение с `Rest` получает остаток `1 − Σ g`.
 * Если сумма остальных больше 1, они нормируются, и остаток равен нулю.
 *
 * Классическая пороговая модель — это `{Rest, Threshold(k1, k2)}`:
 * мнение 1 принимается с вероятностью `f(s)`, иначе вершина получает мнение 0.
 */
using OpinionRule = std::vector<OpinionFunction>;

/// @brief Число мнений в модели. Сейчас мнения бинарные: 0 и 1.
constexpr int NUM_OPINIONS = 2;

/// @brief Значение функции влияния при доле соседей `share`, не меньше нуля.
/// Функция должна быть корректной (`validateOpinionFunction` вернула пустую строку).
double evaluateInfluence(const OpinionFunction& function, double share);

/**
 * @brief Вероятности перейти в каждое мнение при заданных долях соседей.
 * @param rule Правило смены мнений.
 * @param shares Доля соседей с каждым мнением, размер равен `rule.size()`.
 * @return Вероятности по мнениям с суммой 1 или все нули, если вершина должна сохранить мнение.
 */
std::vector<double> adoptionProbabilities(const OpinionRule& rule, const std::vector<double>& shares);

/**
 * @brief Порядок, в котором мнения перебираются при случайном выборе.
 *
 * Сначала мнения по возрастанию номера, мнение с `Rest` последним. Благодаря этому
 * классическая пороговая модель тратит случайные числа так же, как раньше.
 */
std::vector<int> samplingOrder(const OpinionRule& rule);

/**
 * @brief Строит правило смены мнений из одной функции модели.
 *
 * - `threshold(k1, k2)`: классическая пороговая модель `{Rest, Threshold}` — вершина получает
 *   мнение 1 с вероятностью `f(s₁)`, иначе мнение 0;
 * - `moran(δ)`: процесс Морана `{Moran(0), Moran(δ)}` — мнение 1 («мутант») копируется
 *   в `1 + δ` раз охотнее мнения 0;
 * - `polynomial`, `exponential`: одна и та же функция у всех мнений,
 *   `P(k) = g(s_k) / Σ g(s_m)`.
 */
OpinionRule makeRule(const OpinionFunction& function);

/// @brief Пороговая функция с порогами `k1`, `k2`.
OpinionFunction thresholdFunction(double k1, double k2);

/// @brief Функция по умолчанию: пороговая с k1 = 0, k2 = 1.
OpinionFunction defaultOpinionFunction();

/// @brief Функции, которые можно выбрать для модели (без служебной `Rest`).
std::vector<OpinionFunctionKind> allOpinionKinds();
std::string opinionKindToString(OpinionFunctionKind kind);
std::optional<OpinionFunctionKind> opinionKindFromString(const std::string& name);
std::string opinionKindDisplayName(OpinionFunctionKind kind);
std::string opinionParamsDescription(OpinionFunctionKind kind);

/// @brief Текст ошибки, если функцию нельзя использовать в модели, иначе пустая строка.
std::string validateOpinionFunction(const OpinionFunction& function);

/// @brief Текст ошибки, если правило некорректно, иначе пустая строка.
std::string validateOpinionRule(const OpinionRule& rule);

/// @brief Краткая запись функции, например `moran(0.5)`.
std::string describeOpinionFunction(const OpinionFunction& function);

} // namespace od::model
