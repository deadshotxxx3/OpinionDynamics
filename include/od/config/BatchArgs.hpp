#pragma once

#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace od::config {

/**
 * @brief Ошибка в аргументах командной строки.
 *
 * Отличается от прочих ошибок тем, что после сообщения стоит показать справку
 * `printBatchUsage()`.
 */
class UsageError : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};

/// @brief Сырые значения одной группы упрямых вершин из флагов.
struct StubbornArgs {
    int count = 0;
    bool manual = false;
    std::vector<int> targets;
};

/**
 * @brief Функция вероятности рёбер в том виде, как она задана флагами.
 *
 * Имя и параметры ещё не проверены. Устаревшие флаги `--p0`/`--k` (и `--remove-p0`/`--remove-k`)
 * задают линейную функцию `a + b·t`.
 */
struct FunctionArgs {
    std::string name;
    std::string params;
    std::optional<double> legacyA;
    std::optional<double> legacyB;
};

/**
 * @brief Все флаги batch-режима после разбора.
 *
 * Необязательные значения хранятся как `std::optional`: пустое значение означает
 * «флаг не указан», и при загрузке из файла тогда используется значение из файла.
 */
struct BatchArgs {
    bool generate = true;                   ///< `true` — генерация, `false` — загрузка из `loadDir`.

    std::vector<int> levelVertices;         ///< Число вершин по уровням.
    std::vector<double> levelProbabilities; ///< Вероятности рёбер по уровням.
    StubbornArgs stubbornAttach;            ///< Новые упрямые вершины степени 1.
    StubbornArgs stubbornAssign;            ///< Существующие вершины, назначаемые упрямыми.

    std::string loadDir;                    ///< Папка прогона для загрузки.
    bool loadFinalGraph = false;            ///< Загружать `graph_final.txt` вместо `graph_initial.txt`.

    std::optional<double> k1;
    std::optional<double> k2;
    std::optional<int> tMax;
    std::optional<unsigned int> seed;
    std::optional<bool> useWeights;

    std::optional<bool> dynamicEdges;
    FunctionArgs addFunction;
    std::optional<bool> removeEdges;
    FunctionArgs removeFunction;
};

/// @brief Проверяет, есть ли среди аргументов флаг `--batch`.
bool hasBatchFlag(int argc, char** argv);

/**
 * @brief Разбирает аргументы командной строки batch-режима.
 *
 * Проверяет только синтаксис: известен ли флаг, есть ли у него значение, является ли
 * значение числом. Согласованность параметров проверяется позже, при построении запуска.
 *
 * @throws UsageError При неизвестном флаге, отсутствующем значении или незаданных уровнях.
 * @throws std::invalid_argument При некорректном значении флага.
 */
BatchArgs parseBatchArgs(int argc, char** argv);

/// @brief Печатает справку по флагам batch-режима и список функций вероятности.
void printBatchUsage();

} // namespace od::config
