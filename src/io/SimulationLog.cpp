#include "od/io/SimulationLog.hpp"

#include <fstream>
#include <stdexcept>

namespace od::io {

void saveSimulationLog(const SimulationLog& log, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("saveSimulationLog: не удалось открыть файл: " + filename);
    }

    file << "SEED " << log.seed << "\n";
    file << "T_MAX " << log.tMax << "\n";
    file << "K1 " << log.k1 << "\n";
    file << "K2 " << log.k2 << "\n";
    file << "DYNAMIC_EDGES " << (log.dynamicEdges ? 1 : 0) << "\n";
    file << "P0 " << log.p0 << "\n";
    file << "K " << log.k << "\n";
    file << "REMOVE_EDGES " << (log.removeEdges ? 1 : 0) << "\n";
    file << "REMOVE_P0 " << log.removeP0 << "\n";
    file << "REMOVE_K " << log.removeK << "\n";
    file << "USE_WEIGHTS " << (log.useWeights ? 1 : 0) << "\n";
    file << "\n";

    file << "INITIAL_GRAPH_FILE " << log.initialGraphFile << "\n";
    file << "FINAL_GRAPH_FILE " << log.finalGraphFile << "\n";
    file << "\n";

    file << "HISTORY " << log.history.size() << "\n";
    for (const auto& rec : log.history) {
        file << rec.ones << " " << rec.edges << "\n";
    }
    file << "\n";

    file << "OPINION_CHANGES " << log.opinionChanges.size() << "\n";
    for (const auto& ch : log.opinionChanges) {
        file << ch.step << " " << ch.vertex << " " << ch.from << " " << ch.to << "\n";
    }
    file << "\n";

    file << "EDGE_EVENTS " << log.edgeEvents.size() << "\n";
    for (const auto& ev : log.edgeEvents) {
        file << ev.step << " "
             << (ev.added ? "ADD" : "REMOVE") << " "
             << ev.u << " " << ev.v << " " << ev.weight << "\n";
    }
    file << "\n";

    file << "FINAL_OPINIONS " << log.finalOpinions.size() << "\n";
    for (int op : log.finalOpinions) {
        file << op << "\n";
    }

    if (!file.good()) {
        throw std::runtime_error("saveSimulationLog: ошибка при записи в файл: " + filename);
    }
}

} // namespace od::io