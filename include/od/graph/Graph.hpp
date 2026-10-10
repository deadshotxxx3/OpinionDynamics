#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

/// @brief Граф, на котором идёт динамика мнений.
namespace od::graph {

/**
 * @brief Неориентированный взвешенный граф с отметками упрямых вершин.
 *
 * Вершины нумеруются подряд от `0` до `getNumVertices() - 1`. Петли запрещены. Вес ребра лежит в отрезке `[0, 1]`
 * и одинаков в обе стороны.
 *
 * Граф хранится списками смежности: для каждой вершины словарь «сосед → вес».
 * Добавление, удаление и проверка ребра работают в среднем за O(1).
 *
 * Каждая вершина может быть отмечена как упрямая. Сам граф упрямство никак не использует,
 * это отметка для модели мнений: упрямая вершина всегда имеет мнение 1.
 *
 * Все методы, принимающие номер вершины, бросают `std::out_of_range`,
 * если номер вне диапазона `[0, getNumVertices())`.
 *
 * @code
 * od::graph::Graph graph(3);
 * graph.addEdge(0, 1, 0.5);   // добавляет ребро между 0 и 1 с весом 0.5
 * graph.addEdge(1, 2, 0.8);
 * graph.setStubborn(2, true); // устанавливает вершину 2, как упрямую
 *
 * graph.getNumEdges();     // 2
 * graph.getWeight(1, 0);   // 0.5
 * graph.hasEdge(1,2);      // вернет true
 * graph.removeEdge(0, 1);  // вернёт 0.5
 * graph.isStubborn(2);     // вернет true
 * @endcode
 */
class Graph {
public:
    /// @brief Соседи вершины: номер соседа → вес ребра.
    using Neighbors = std::unordered_map<int, double>;

    /**
     * @brief Создаёт граф из `numVertices` вершин без рёбер и без упрямых вершин.
     * @param numVertices Число вершин, может быть 0.
     * @throws std::invalid_argument Если `numVertices < 0`.
     */
    explicit Graph(int numVertices);

    /**
     * @brief Добавляет в конец новую изолированную обычную вершину.
     * @return Номер новой вершины, равный прежнему `getNumVertices()`.
     */
    int addVertex();

    /**
     * @brief Добавляет ребро между `from` и `to` или меняет вес существующего.
     *
     * Если ребро уже есть, обновляется только вес, число рёбер не меняется.
     * Порядок вершин не важен: `addEdge(a, b, w)` и `addEdge(b, a, w)` дают одно и то же ребро.
     *
     * @param from Первый конец ребра.
     * @param to Второй конец ребра.
     * @param weight Вес ребра из отрезка `[0, 1]`.
     * @throws std::out_of_range Если номер вершины вне диапазона.
     * @throws std::invalid_argument Если `from == to` или вес вне `[0, 1]`, в том числе `NaN`.
     *         Граф при этом не меняется.
     */
    void addEdge(int from, int to, double weight);

    /**
     * @brief Удаляет ребро между `from` и `to`, если оно есть.
     * @return Вес удалённого ребра или `std::nullopt`, если ребра не было.
     * @throws std::out_of_range Если номер вершины вне диапазона.
     * @throws std::invalid_argument Если `from == to`.
     */
    std::optional<double> removeEdge(int from, int to);

    /**
     * @brief Проверяет, есть ли ребро между `from` и `to`.
     * @throws std::out_of_range Если номер вершины вне диапазона.
     */
    bool hasEdge(int from, int to) const;

    /**
     * @brief Возвращает вес ребра между `from` и `to`.
     * @return Вес ребра или `std::nullopt`, если ребра нет.
     * @throws std::out_of_range Если номер вершины вне диапазона.
     */
    std::optional<double> getWeight(int from, int to) const;

    /// @brief Число вершин, включая упрямые.
    int getNumVertices() const noexcept {
        return static_cast<int>(adjacency_.size());
    }

    /// @brief Число рёбер. Каждое неориентированное ребро считается один раз.
    long long getNumEdges() const noexcept {
        return numEdges_;
    }

    /**
     * @brief Возвращает всех соседей вершины вместе с весами рёбер.
     *
     * После `addVertex` ссылку нужно получить заново. Итераторы по соседям становятся
     * недействительными, если рёбра этой вершины меняются во время обхода.
     *
     * @throws std::out_of_range Если номер вершины вне диапазона.
     */
    const Neighbors& getNeighbors(int vertex) const;

    /**
     * @brief Отмечает вершину как упрямую (`true`) или снимает отметку (`false`).
     * @throws std::out_of_range Если номер вершины вне диапазона.
     */
    void setStubborn(int vertex, bool value);

    /**
     * @brief Проверяет, отмечена ли вершина как упрямая.
     * @throws std::out_of_range Если номер вершины вне диапазона.
     */
    bool isStubborn(int vertex) const;

private:
    std::size_t index(int vertex) const;
    void checkEdgeEnds(int from, int to) const;

    std::vector<Neighbors> adjacency_;
    std::vector<std::uint8_t> stubborn_;
    long long numEdges_ = 0;
};

} // namespace od::graph