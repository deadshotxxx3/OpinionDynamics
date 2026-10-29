#pragma once

#include "od/io/GraphIO.hpp"
#include "od/model/DynamicsModel.hpp"
#include "od/model/EdgeEvolution.hpp"

#include <string>
#include <vector>

namespace od::io {

/// @brief Состояние после одного шага: число рёбер и сколько вершин с каждым мнением.
struct StepRecord {
    long long edges;                             ///< Число рёбер после шага.
    std::vector<od::model::OpinionCount> counts; ///< Только мнения, которые есть у вершин, от самого частого.
};

/// @brief Смена мнения одной вершины.
struct OpinionChange {
    int step;   ///< Номер шага, с 1.
    int vertex; ///< Номер вершины.
    int from;   ///< Мнение до шага.
    int to;     ///< Мнение после шага.
};

/**
 * @brief Всё, что пишется в `simulation_log.txt`.
 *
 * Формат: строки PARAMS (как в файле графа), ссылки на файлы графов, затем секции
 * `OPINION_HISTORY` (на каждом шаге: число рёбер, число мнений и пары «мнение число_вершин»),
 * `OPINION_CHANGES`, `EDGE_EVENTS` и `FINAL_OPINIONS`.
 */
struct SimulationLog {
    SimulationParams params;

    std::string initialGraphFile;
    std::string finalGraphFile;

    std::vector<StepRecord> history;
    std::vector<OpinionChange> opinionChanges;
    std::vector<od::model::EdgeEvent> edgeEvents;
    std::vector<int> finalOpinions;
};

/**
 * @brief Сохраняет лог прогона в текстовый файл.
 * @throws std::runtime_error Если файл не открывается или запись не удалась.
 */
void saveSimulationLog(const SimulationLog& log, const std::string& filename);

} // namespace od::io
