#pragma once

#include "od/graph/Graph.hpp"
#include "od/model/OpinionFunction.hpp"

#include <random>
#include <vector>

/// @brief Модель: мнения, рёбра и функции вероятности.
namespace od::model {

using od::graph::Graph;

/// @brief Сколько вершин держат мнение.
struct OpinionCount {
    int opinion; ///< Номер мнения.
    int count;   ///< Число вершин с этим мнением.
};

/**
 * @brief Считает вершины по мнениям.
 * @param opinions Мнения вершин, каждое от 0 до `numOpinions − 1`.
 * @param numOpinions Число мнений.
 * @return Только мнения, которые есть хотя бы у одной вершины, по убыванию числа вершин,
 *         при равенстве — по возрастанию номера мнения.
 */
std::vector<OpinionCount> countOpinions(const std::vector<int>& opinions, int numOpinions);

/// @brief Как вершина выбирает новое мнение.
struct ChoiceSettings {
    ChoiceRule rule = ChoiceRule::Max; ///< Правило выбора.
    std::vector<int> popularity;                ///< Мнения от самого популярного, решают ничьи в правиле `Max`.
};

/**
 * @brief Обновляет мнения всех вершин за один шаг.
 *
 * Для каждой обычной вершины с соседями считаются доли соседей по мнениям — только по тем мнениям,
 * которые есть у соседей. Поэтому шаг занимает время порядка числа рёбер и не зависит от числа мнений.
 * По долям и правилу смены мнений считаются вероятности `P(k)` (`adoptionProbabilities`),
 * затем новое мнение выбирается по правилу выбора:
 *
 * - `Proportional`: одно случайное число, мнение `k` выбирается с вероятностью `P(k)`;
 * - `Max`: кандидат — мнение с наибольшей `P(k)`. Если таких несколько, побеждает то, что выше
 *   в списке популярности, а если ни одного из них в списке нет — случайное из них.
 *   Затем «монетка»: вершина переходит в кандидата с вероятностью `P(кандидата)`, иначе
 *   сохраняет мнение. Если кандидат совпадает с текущим мнением, монетка не нужна.
 *
 * Если все вероятности нулевые, вершина сохраняет мнение. Упрямые вершины и вершины
 * без соседей (или с нулевым суммарным весом рёбер) мнение не меняют.
 *
 * Объект хранит рабочие буферы, поэтому его стоит создать один раз на прогон.
 */
class OpinionUpdater {
public:
    /**
     * @param rule Правило смены мнений, число мнений — `rule.functions.size()`.
     * @param choice Правило выбора и список популярности.
     * @param useWeights Считать соседей с весами рёбер или все одинаково.
     * @throws std::invalid_argument Если правило не проходит `validateOpinionRule` или в списке
     *         популярности есть мнение вне диапазона или повтор.
     */
    OpinionUpdater(OpinionRule rule, const ChoiceSettings& choice, bool useWeights);

    /// @brief Число мнений в правиле.
    int numOpinions() const noexcept {
        return static_cast<int>(rule_.functions.size());
    }

    /**
     * @brief Один синхронный шаг мнений: все вершины смотрят на мнения предыдущего шага.
     * @param graph Граф.
     * @param current Мнения на предыдущем шаге, каждое от 0 до `numOpinions() − 1`.
     * @param rng Генератор случайных чисел прогона.
     * @return Мнения после шага.
     */
    std::vector<int> step(const Graph& graph, const std::vector<int>& current, std::mt19937& rng);

private:
    const std::vector<OpinionShare>& neighborShares(const Graph& graph, int vertex, const std::vector<int>& opinions);
    int chooseMax(const std::vector<OpinionChance>& chances, int current, std::mt19937& rng);
    int breakTie(std::mt19937& rng);

    OpinionRule rule_;
    ChoiceRule choiceRule_;
    std::vector<int> popularityRank_;
    bool useWeights_;

    std::vector<double> weightByOpinion_;
    std::vector<char> seen_;
    std::vector<int> touched_;
    std::vector<OpinionShare> shares_;
    std::vector<int> tied_;
};

} // namespace od::model
