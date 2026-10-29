#pragma once

#include "od/graph/Graph.hpp"

#include <optional>
#include <string>
#include <vector>

namespace od::model {

/**
 * @brief Начальное мнение группы вершин: одно и то же у всех или случайное.
 *
 * Случайное мнение выбирается равномерно из `0..n − 1` отдельно для каждой вершины.
 */
struct InitialGroup {
    bool random = false; ///< `true` — случайное мнение, `false` — у всех `value`.
    int value = 0;       ///< Мнение всей группы, если `random == false`.
};

/// @brief Мнение, заданное вручную для одной вершины.
struct ManualOpinion {
    int vertex;  ///< Номер вершины.
    int opinion; ///< Её начальное мнение.
};

/**
 * @brief Как выбрать начальные мнения всех вершин.
 *
 * Сначала мнения получают группы (обычные и упрямые вершины), затем поверх них
 * записываются мнения, заданные вручную. Упрямые вершины сохраняют начальное мнение
 * всю симуляцию.
 *
 * По умолчанию обычные вершины начинают с мнения 0, упрямые — с мнения 1,
 * как в бинарной модели.
 *
 * Типичные постановки:
 * - **упрямое меньшинство:** обычные — одно мнение, упрямые — другое. Вопрос: захватит ли
 *   мнение упрямых весь граф и при какой их доле;
 * - **случайный старт:** все мнения случайные, упрямых нет. Вопрос: придёт ли граф к одному
 *   мнению (консенсусу), за сколько шагов и сколько мнений выживет;
 * - **конкуренция упрямых:** упрямые со случайными мнениями — несколько «пропагандистов»
 *   разных мнений тянут граф в разные стороны.
 */
struct InitialOpinionSettings {
    InitialGroup regular{false, 0};     ///< Обычные вершины.
    InitialGroup stubborn{false, 1};    ///< Упрямые вершины.
    std::vector<ManualOpinion> manual;  ///< Мнения отдельных вершин, перекрывают группы.
};

/**
 * @brief Строит начальные мнения всех вершин графа.
 *
 * Для случайных мнений у программы свой отдельный источник случайных чисел, который
 * зависит только от `seed`. Поэтому включение случайных мнений не меняет ни граф,
 * ни случайные числа симуляции. Если случайных групп нет, случайные числа не тратятся вовсе.
 *
 * @param graph Граф, упрямые вершины в нём уже отмечены.
 * @param settings Настройки начальных мнений, должны пройти `validateInitialSettings`.
 * @param numOpinions Число мнений.
 * @param seed Seed прогона.
 * @throws std::invalid_argument Если вершина из ручного списка не существует в графе.
 */
std::vector<int> makeInitialOpinions(const od::graph::Graph& graph, const InitialOpinionSettings& settings,
                                     int numOpinions, unsigned int seed);

/// @brief Текст ошибки, если в настройках мнение вне `0..numOpinions − 1`, отрицательная
/// вершина или вершина в ручном списке дважды, иначе пустая строка.
std::string validateInitialSettings(const InitialOpinionSettings& settings, int numOpinions);

/// @brief Текст ошибки, если мнения нельзя использовать для графа из `numVertices` вершин
/// с `numOpinions` мнениями, иначе пустая строка.
std::string validateOpinionVector(const std::vector<int>& opinions, int numVertices, int numOpinions);

/// @brief Запись группы в файлах и флагах: `random` или номер мнения.
std::string initialGroupToString(const InitialGroup& group);

/// @brief Разбирает `random` или неотрицательное целое. Диапазон мнения не проверяется.
std::optional<InitialGroup> initialGroupFromString(const std::string& text);

/// @brief Запись ручных мнений в виде `вершина:мнение,вершина:мнение`.
std::string manualOpinionsToString(const std::vector<ManualOpinion>& manual);

/// @brief Разбирает запись `вершина:мнение,вершина:мнение`. Пустая строка — пустой список.
std::optional<std::vector<ManualOpinion>> manualOpinionsFromString(const std::string& text);

/// @brief Описание группы для вывода: «случайно» или «мнение 3».
std::string describeInitialGroup(const InitialGroup& group);

} // namespace od::model
