#pragma once

/// @brief Конфигурация запуска: параметры, флаги, интерактивный ввод.
namespace od::config {

/// @brief Наибольшее число вершин в графе, включая прикреплённые упрямые.
constexpr int MAX_VERTICES = 20000;

/// @brief Наибольшее число уровней графа.
constexpr int MAX_LEVELS = 10000;

/// @brief Наибольшее число шагов симуляции.
constexpr int MAX_STEPS = 100000;

/// @brief Сколько упрямых вершин можно выбрать вручную в интерактивном режиме.
constexpr int MAX_MANUAL_TARGETS = 100;

/// @brief Наибольшая степень полинома при вводе функции в интерактивном режиме.
constexpr int MAX_POLYNOMIAL_DEGREE = 10;

} // namespace od::config
