#pragma once

#include "od/config/RuntimeParams.hpp"

#include <vector>

namespace od::model {

/**
 * @brief Проводит прогон симуляции и сохраняет результаты в новую папку прогона.
 *
 * Папка называется `<runsDir>/run_<seed>` (по умолчанию `runs/run_<seed>`). Если она уже существует,
 * выбирается `run_<seed>_2`, `run_<seed>_3` и так далее, существующие прогоны не перезаписываются.
 * В папку пишутся `graph_initial.txt` (граф, начальные мнения и параметры), `graph_final.txt`
 * (граф и мнения в конце) и `simulation_log.txt`.
 *
 * Каждый шаг сначала меняет рёбра (если включена динамика рёбер), затем синхронно
 * обновляет мнения всех обычных вершин. Ход прогона печатается в консоль.
 *
 * @param graph Начальный граф, изменяется по ходу симуляции.
 * @param params Параметры прогона. Если seed не задан, он выбирается случайно.
 * @param initialOpinions Готовые начальные мнения или пусто — тогда они строятся
 *        по `params.initialOpinions`.
 * @param output Папка для прогонов и подробность вывода в консоль.
 * @throws std::invalid_argument Если параметры не проходят `od::config::validateParams`
 *         или готовые мнения не подходят к графу и числу мнений.
 */
void runSimulation(od::graph::Graph graph, od::config::RuntimeParams params, std::vector<int> initialOpinions,
                   const od::config::OutputOptions& output);

} // namespace od::model
