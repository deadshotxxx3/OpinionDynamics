#pragma once

#include "od/config/ManualGenConfig.hpp"
#include "od/graph/Graph.hpp"
#include "od/io/GraphIO.hpp"
#include "od/model/ProbabilityFunction.hpp"

#include <string>

namespace od::config {

/// @brief Параметры одного прогона симуляции.
struct RuntimeParams {
    double k1 = 0.0;            ///< Нижний порог функции смены мнения.
    double k2 = 1.0;            ///< Верхний порог функции смены мнения.
    bool useWeights = false;    ///< Учитывать веса рёбер при подсчёте доли соседей.

    bool dynamicEdges = false;                       ///< Включено появление рёбер.
    od::model::ProbabilityFunction edgeAddFunction;  ///< Вероятность появления ребра p(t).
    bool removeEdges = false;                        ///< Включено исчезновение рёбер.
    od::model::ProbabilityFunction edgeRemoveFunction; ///< Вероятность исчезновения ребра p(t).

    int tMax = 500;             ///< Число шагов симуляции.
    unsigned int seed = 0;      ///< Seed прогона, действует только при `seedSpecified`.
    bool seedSpecified = false; ///< `false` — seed будет выбран случайно.
};

/// @brief Всё, что нужно для запуска: граф и параметры.
struct SimulationSetup {
    od::graph::Graph graph;
    RuntimeParams params;
};

/// @brief Берёт параметры модели и динамики рёбер из конфигурации генерации.
/// Число шагов и seed остаются по умолчанию.
RuntimeParams paramsFromConfig(const ManualGenConfig& config);

/// @brief Берёт все параметры, включая seed и число шагов, из секции PARAMS файла графа.
RuntimeParams paramsFromFile(const od::io::SimulationParams& fileParams);

/// @brief Превращает параметры в формат секции PARAMS для сохранения в файл.
od::io::SimulationParams toFileParams(const RuntimeParams& params);

/// @brief Если seed не задан, выбирает его случайно через `std::random_device`.
void ensureSeed(RuntimeParams& params);

/**
 * @brief Проверяет, что параметры допустимы для запуска.
 * @throws std::invalid_argument Если пороги не удовлетворяют `0 ≤ k1 < k2 ≤ 1`,
 *         число шагов меньше 1 или функция вероятности рёбер некорректна.
 */
void validateParams(const RuntimeParams& params);

/// @brief Печатает настройки динамики рёбер, каждая строка начинается с `indent`.
void printEdgeDynamics(const RuntimeParams& params, const std::string& indent);

} // namespace od::config
