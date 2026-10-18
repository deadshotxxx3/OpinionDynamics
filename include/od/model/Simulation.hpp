#pragma once

#include "od/config/RuntimeParams.hpp"

namespace od::model {

/**
 * @brief Проводит прогон симуляции и сохраняет результаты в новую папку прогона.
 *
 * Папка называется `runs/run_<seed>`. Если она уже существует, выбирается
 * `runs/run_<seed>_2`, `runs/run_<seed>_3` и так далее, существующие прогоны не перезаписываются.
 * В папку пишутся `graph_initial.txt` (граф и параметры), `graph_final.txt` и `simulation_log.txt`.
 *
 * Каждый шаг сначала меняет рёбра (если включена динамика рёбер), затем синхронно
 * обновляет мнения всех обычных вершин. Ход прогона печатается в консоль.
 *
 * @param graph Начальный граф, изменяется по ходу симуляции.
 * @param params Параметры прогона. Если seed не задан, он выбирается случайно.
 * @throws std::invalid_argument Если параметры не проходят `od::config::validateParams`.
 */
void runSimulation(od::graph::Graph graph, od::config::RuntimeParams params);

} // namespace od::model
