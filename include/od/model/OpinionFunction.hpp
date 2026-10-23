#pragma once

#include <optional>
#include <string>
#include <vector>

namespace od::model {

/// @brief Вид функции влияния мнения.
enum class OpinionFunctionKind {
    Threshold,   ///< Пороговая: 0 при s ≤ k1, линейно до 1 на (k1, k2], 1 при s > k2.
    Moran,       ///< Процесс Морана: (1 + δ)·s у мнения-мутанта, s у остальных мнений.
    Polynomial,  ///< Полиномиальная: a·sⁿ + b·sⁿ⁻¹ + … + свободный член, коэффициенты от старшей степени.
                 ///< Линейная функция a + b·s — это полином первой степени с коэффициентами `b, a`.
    Exponential, ///< Экспоненциальная: a·e^(b·s).
    Rest         ///< Служебная: вся вероятность, не занятая другими мнениями. Пользователь её не выбирает.
};

/**
 * @brief Как вершина выбирает новое мнение по вероятностям `P(k)`.
 *
 * Вероятности считаются только для мнений, которые есть у соседей вершины.
 * Новые прогоны по умолчанию используют `Max`. Файлы без строки `CHOICE_RULE`
 * (сохранённые до появления правил) читаются как `Proportional`, так они воспроизводятся.
 */
enum class ChoiceRule {
    Proportional, ///< Мнение `k` выбирается случайно с вероятностью `P(k)`, как в бинарной модели.
    Max           ///< Кандидат — мнение с наибольшей `P(k)`, вершина переходит в него с вероятностью `P(кандидата)`.
};

/// @brief Число мнений по умолчанию: бинарная модель с мнениями 0 и 1.
constexpr int DEFAULT_NUM_OPINIONS = 2;

/// @brief Наибольшее допустимое число мнений.
constexpr int MAX_OPINIONS = 10000;

/// @brief Мнение-мутант процесса Морана, если оно не задано вторым параметром.
constexpr int DEFAULT_MORAN_MUTANT = 1;

/**
 * @brief Функция изменения мнений.
 *
 * Аргумент функции — доля `s` соседей вершины, у которых данное мнение (с весами рёбер или без).
 * Значение — «сила притяжения» мнения: чем оно больше, тем вероятнее вершина его примет.
 * Отрицательные значения и `NaN` считаются нулём.
 *
 * Сейчас в модели одна функция на все мнения, её превращает в правило `makeRule`.
 * У функции Морана необязательный второй параметр — номер мнения-мутанта.
 */
struct OpinionFunction {
    OpinionFunctionKind kind = OpinionFunctionKind::Threshold;
    std::vector<double> params = {0.0, 1.0};
};

/**
 * @brief Правило смены мнений: функция влияния для каждого мнения.
 *
 * Это внутреннее представление, рассчитанное на будущее расширение, когда у каждого мнения
 * будет своя функция. Сейчас правило строится из одной функции через `makeRule`.
 *
 * Вершина считает `g_k(s_k)` для мнений, которые есть у её соседей, и вероятности
 * `P(k) = g_k / Σ g`. Если у одного из мнений функция `Rest`, значения остальных функций
 * считаются вероятностями напрямую, а мнение с `Rest` получает остаток `1 − Σ g`
 * (даже если его нет у соседей). Если сумма остальных больше 1, они нормируются,
 * и остаток равен нулю.
 */
struct OpinionRule {
    std::vector<OpinionFunction> functions; ///< Функция мнения `k` — `functions[k]`, размер равен числу мнений.
    int restOpinion = -1;                   ///< Мнение с функцией `Rest` или −1, если такого нет.
};

/// @brief Доля соседей вершины с одним мнением.
struct OpinionShare {
    int opinion;  ///< Номер мнения.
    double share; ///< Доля соседей с этим мнением, от 0 до 1.
};

/// @brief Вероятность перейти в мнение.
struct OpinionChance {
    int opinion;        ///< Номер мнения.
    double probability; ///< Вероятность перехода.
};

/// @brief Значение функции влияния при доле соседей `share`, не меньше нуля.
/// Функция должна быть корректной (`validateOpinionFunction` вернула пустую строку).
double evaluateInfluence(const OpinionFunction& function, double share);

/**
 * @brief Вероятности перейти в мнения при заданных долях соседей.
 *
 * Учитываются только мнения из `shares`, то есть те, что есть у соседей,
 * и мнение с функцией `Rest`, если оно есть в правиле.
 *
 * @param rule Правило смены мнений.
 * @param shares Доли мнений соседей по возрастанию номера мнения, номера меньше `rule.functions.size()`.
 * @return Вероятности в порядке розыгрыша: мнения из `shares` по возрастанию, мнение с `Rest` последним.
 *         Сумма равна 1 или все вероятности нулевые — тогда вершина сохраняет мнение.
 */
std::vector<OpinionChance> adoptionProbabilities(const OpinionRule& rule, const std::vector<OpinionShare>& shares);

/**
 * @brief Строит правило смены мнений из одной функции модели.
 *
 * - `threshold(k1, k2)` при правиле `Proportional` — классическая пороговая модель: мнение 0
 *   получает остаток (`Rest`), остальные мнения `k` принимаются с вероятностью `f(s_k)`.
 *   При двух мнениях это «мнение 1 с вероятностью `f(s₁)`, иначе 0». При правиле `Max`
 *   функция `f` одна у всех мнений, как у остальных функций;
 * - `moran(δ[, мутант])`: у мнения-мутанта (по умолчанию 1) `(1 + δ)·s`, у остальных `s`;
 * - `polynomial`, `exponential`: одна и та же функция у всех мнений.
 *
 * @param function Функция модели, должна пройти `validateOpinionSettings` для `numOpinions`.
 * @param numOpinions Число мнений, от 2 до `MAX_OPINIONS`.
 * @param choice Правило выбора нового мнения.
 */
OpinionRule makeRule(const OpinionFunction& function, int numOpinions, ChoiceRule choice);

/// @brief Пороговая функция с порогами `k1`, `k2`.
OpinionFunction thresholdFunction(double k1, double k2);

/// @brief Функция по умолчанию: пороговая с k1 = 0, k2 = 1.
OpinionFunction defaultOpinionFunction();

/// @brief Мнение-мутант функции Морана: второй параметр или `DEFAULT_MORAN_MUTANT`.
int moranMutant(const OpinionFunction& function);

/// @brief Функции, которые можно выбрать для модели (без служебной `Rest`).
std::vector<OpinionFunctionKind> allOpinionKinds();
std::string opinionKindToString(OpinionFunctionKind kind);
std::optional<OpinionFunctionKind> opinionKindFromString(const std::string& name);
std::string opinionKindDisplayName(OpinionFunctionKind kind);
std::string opinionParamsDescription(OpinionFunctionKind kind);

/// @brief Имя правила выбора в файлах и флагах: `proportional` или `max`.
std::string choiceRuleToString(ChoiceRule rule);
std::optional<ChoiceRule> choiceRuleFromString(const std::string& name);

/// @brief Название правила выбора для вывода: «пропорционально» или «максимум».
std::string choiceRuleDisplayName(ChoiceRule rule);

/// @brief Текст ошибки, если функцию нельзя использовать в модели, иначе пустая строка.
std::string validateOpinionFunction(const OpinionFunction& function);

/// @brief Текст ошибки, если в списке популярности есть мнение вне `0..numOpinions − 1` или повтор.
std::string validatePopularity(const std::vector<int>& popularity, int numOpinions);

/**
 * @brief Проверяет функцию и список популярности для модели с `numOpinions` мнениями.
 *
 * Кроме `validateOpinionFunction` проверяет, что мутант Морана меньше `numOpinions`,
 * а в списке популярности мнения от 0 до `numOpinions − 1` без повторов.
 *
 * @return Текст ошибки или пустая строка.
 */
std::string validateOpinionSettings(const OpinionFunction& function, const std::vector<int>& popularity,
                                    int numOpinions);

/// @brief Текст ошибки, если правило некорректно, иначе пустая строка.
std::string validateOpinionRule(const OpinionRule& rule);

/// @brief Краткая запись функции, например `moran(0.5)`.
std::string describeOpinionFunction(const OpinionFunction& function);

/// @brief Список мнений через запятую, например `5, 3, 2`.
std::string describeOpinionList(const std::vector<int>& opinions);

} // namespace od::model
