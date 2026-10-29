#pragma once

#include <optional>
#include <string>
#include <vector>

namespace od::model {

/**
 * @brief Вид функции вероятности `p(t)` для динамики рёбер, `t` — номер шага с 0.
 *
 * Задаёт, как со временем меняется вероятность появления (или исчезновения) ребра.
 */
enum class FunctionKind {
    Constant,    ///< `p = a`: сеть меняется с постоянной скоростью.
    Linear,      ///< `p = a + b·t`: скорость равномерно растёт (или падает при `b < 0`).
    Polynomial,  ///< `p = c₀ + c₁·t + c₂·t² + …`: коэффициенты от младшей степени к старшей.
    Exponential, ///< `p = a·e^(b·t)`: быстрый рост при `b > 0` или затухание при `b < 0`.
    Saturation,  ///< `p = pmax·(1 − e^(−r·t))`: начинается с 0 и выходит на `pmax`, `r` — скорость выхода.
    Logistic,    ///< `p = pmax / (1 + e^(−r·(t − t₀)))`: S-образный рост, в момент `t₀` половина от `pmax`.
    Step,        ///< `p = a` при `t < T`, иначе `b`: резкое изменение в момент `T` (например, событие).
    Periodic     ///< `p = a + b·sin(w·t)`: колебания с периодом `2π / w` (например, суточный или сезонный ритм).
};

/**
 * @brief Функция вероятности рёбер: вид и параметры.
 *
 * Внимание: у полинома здесь коэффициенты идут от младшей степени (`c₀, c₁, …`),
 * а у полиномиальной функции изменения мнений — от старшей.
 */
struct ProbabilityFunction {
    FunctionKind kind = FunctionKind::Linear;
    std::vector<double> params = {0.0, 0.0};
};

/**
 * @brief Значение функции на шаге `t`, обрезанное до отрезка `[0, 1]`.
 *
 * Отрицательные значения и `NaN` дают 0, значения больше 1 — 1. Если функция
 * некорректна (не проходит `validateFunction`), результат 0.
 */
double evaluate(const ProbabilityFunction& function, int t);

/// @brief Все виды функций в порядке показа в меню.
std::vector<FunctionKind> allKinds();

/// @brief Имя вида для файлов и флагов, например `logistic`.
std::string kindToString(FunctionKind kind);

/// @brief Вид по имени или `std::nullopt`, если имя неизвестно.
std::optional<FunctionKind> kindFromString(const std::string& name);

/// @brief Название с формулой для меню, например «Логистическая: p = …».
std::string kindDisplayName(FunctionKind kind);

/// @brief Список параметров вида, например `pmax, r, t0`.
std::string paramsDescription(FunctionKind kind);

/// @brief Текст ошибки, если число параметров не подходит или они не конечны, иначе пустая строка.
std::string validateFunction(const ProbabilityFunction& function);

/// @brief Параметры через запятую, например `0.02, 0.1, 50`.
std::string formatParams(const std::vector<double>& params);

/// @brief Краткая запись функции, например `logistic(0.02, 0.1, 50)`.
std::string describeFunction(const ProbabilityFunction& function);

} // namespace od::model
