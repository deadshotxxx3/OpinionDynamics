#pragma once

#include "od/graph/Graph.hpp"
#include "od/model/OpinionFunction.hpp"

#include <random>
#include <vector>

/// @brief Модель: мнения, рёбра и функции вероятности.
namespace od::model {

using od::graph::Graph;

/// @brief Начальные мнения: у упрямых вершин 1, у остальных 0.
std::vector<int> initOpinions(const Graph& graph);

/**
 * @brief Доли соседей вершины по мнениям.
 * @param graph Граф.
 * @param vertex Вершина, для которой считаются доли.
 * @param opinions Текущие мнения всех вершин.
 * @param numOpinions Число мнений.
 * @param useWeights Считать соседей с весами рёбер или все одинаково.
 * @return Вектор размера `numOpinions` с суммой 1 или пустой вектор, если соседей нет
 *         или их суммарный вес равен нулю.
 */
std::vector<double> neighborShares(const Graph& graph, int vertex, const std::vector<int>& opinions,
                                   int numOpinions, bool useWeights);

/**
 * @brief Один синхронный шаг мнений: все вершины смотрят на мнения предыдущего шага.
 *
 * Для каждой обычной вершины с соседями считаются доли соседей по мнениям, по `rule`
 * вычисляются вероятности перейти в каждое мнение, и новое мнение выбирается одним
 * случайным числом. Упрямые вершины и вершины без соседей сохраняют мнение.
 *
 * @param graph Граф.
 * @param currentOpinions Мнения на предыдущем шаге.
 * @param rule Правило смены мнений, должно пройти `validateOpinionRule`.
 * @param useWeights Считать соседей с весами рёбер или все одинаково.
 * @param rng Генератор случайных чисел прогона.
 * @return Мнения после шага.
 */
std::vector<int> stepOpinions(
    const Graph& graph,
    const std::vector<int>& currentOpinions,
    const OpinionRule& rule,
    bool useWeights,
    std::mt19937& rng);

} // namespace od::model
