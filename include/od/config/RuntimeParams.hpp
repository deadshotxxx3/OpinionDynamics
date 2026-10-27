#pragma once

#include "od/config/ManualGenConfig.hpp"
#include "od/graph/Graph.hpp"
#include "od/io/GraphIO.hpp"
#include "od/model/InitialOpinions.hpp"
#include "od/model/OpinionFunction.hpp"
#include "od/model/ProbabilityFunction.hpp"

#include <string>
#include <vector>

namespace od::config {

/// @brief Параметры одного прогона симуляции.
struct RuntimeParams {
    int numOpinions = od::model::DEFAULT_NUM_OPINIONS; ///< Число мнений, мнения от 0 до `numOpinions − 1`.
    od::model::OpinionFunction opinionFunction = od::model::defaultOpinionFunction(); ///< Функция изменения мнений.
    od::model::ChoiceRule choiceRule = od::model::ChoiceRule::Max; ///< Правило выбора нового мнения.
    std::vector<int> popularity; ///< Мнения от самого популярного, решают ничьи в правиле «максимум».
    od::model::InitialOpinionSettings initialOpinions; ///< Как выбрать начальные мнения.
    bool useWeights = false;    ///< Учитывать веса рёбер при подсчёте доли соседей.

    bool dynamicEdges = false;                       ///< Включено появление рёбер.
    od::model::ProbabilityFunction edgeAddFunction;  ///< Вероятность появления ребра p(t).
    bool removeEdges = false;                        ///< Включено исчезновение рёбер.
    od::model::ProbabilityFunction edgeRemoveFunction; ///< Вероятность исчезновения ребра p(t).

    int tMax = 500;             ///< Число шагов симуляции.
    unsigned int seed = 0;      ///< Seed прогона, действует только при `seedSpecified`.
    bool seedSpecified = false; ///< `false` — seed будет выбран случайно.
};

/// @brief Куда и как подробно выводить результаты прогона. На саму модель не влияет.
struct OutputOptions {
    std::string runsDir = "runs"; ///< Папка, в которой создаётся папка прогона `run_<seed>`.
    bool quiet = false;           ///< Печатать только папку прогона и итог, без шагов и параметров.
};

/**
 * @brief Всё, что нужно для запуска: граф, параметры и, возможно, готовые начальные мнения.
 *
 * Если `initialOpinions` пуст, мнения строятся по `params.initialOpinions`.
 * Готовые мнения берутся из файла графа при загрузке прогона.
 */
struct SimulationSetup {
    od::graph::Graph graph;
    RuntimeParams params;
    std::vector<int> initialOpinions;
    OutputOptions output;
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
 * @throws std::invalid_argument Если число мнений вне `2..MAX_OPINIONS`, функция изменения мнений,
 *         список популярности или начальные мнения не подходят к числу мнений, число шагов
 *         вне `1..100000` или функция вероятности рёбер некорректна.
 */
void validateParams(const RuntimeParams& params);

/// @brief Текст ошибки, если число мнений вне `2..MAX_OPINIONS`, иначе пустая строка.
std::string validateNumOpinions(int numOpinions);

/// @brief Печатает число мнений, функцию, правило выбора и начальные мнения,
/// каждая строка начинается с `indent`.
void printOpinionModel(const RuntimeParams& params, const std::string& indent);

/// @brief Печатает настройки динамики рёбер, каждая строка начинается с `indent`.
void printEdgeDynamics(const RuntimeParams& params, const std::string& indent);

} // namespace od::config
