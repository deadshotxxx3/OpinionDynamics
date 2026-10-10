#include "od/io/SimulationLog.hpp"
#include "od/io/GraphIO.hpp"

#include <fstream>
#include <iomanip>
#include <stdexcept>

namespace od::io {

namespace {

void writeParams(std::ofstream& file, const SimulationLog& log) {
    file << "SEED " << log.seed << "\n";
    file << "T_MAX " << log.tMax << "\n";
    file << "K1 " << std::setprecision(15) << log.k1 << "\n";
    file << "K2 " << std::setprecision(15) << log.k2 << "\n";
    file << "USE_WEIGHTS " << (log.useWeights ? 1 : 0) << "\n";
    file << "DYNAMIC_EDGES " << (log.dynamicEdges ? 1 : 0) << "\n";
    file << "ADD_FUNCTION ";
    writeFunction(file, log.edgeAddFunction);
    file << "\n";
    file << "REMOVE_EDGES " << (log.removeEdges ? 1 : 0) << "\n";
    file << "REMOVE_FUNCTION ";
    writeFunction(file, log.edgeRemoveFunction);
    file << "\n\n";

    file << "INITIAL_GRAPH_FILE " << log.initialGraphFile << "\n";
    file << "FINAL_GRAPH_FILE " << log.finalGraphFile << "\n\n";
}

void writeHistory(std::ofstream& file, const SimulationLog& log) {
    file << "HISTORY " << log.history.size() << "\n";
    for (const auto& rec : log.history) {
        file << rec.ones << " " << rec.edges << "\n";
    }
    file << "\n";
}

void writeOpinionChanges(std::ofstream& file, const SimulationLog& log) {
    file << "OPINION_CHANGES " << log.opinionChanges.size() << "\n";
    for (const auto& ch : log.opinionChanges) {
        file << ch.step << " " << ch.vertex << " " << ch.from << " " << ch.to << "\n";
    }
    file << "\n";
}

void writeEdgeEvents(std::ofstream& file, const SimulationLog& log) {
    file << "EDGE_EVENTS " << log.edgeEvents.size() << "\n";
    for (const auto& ev : log.edgeEvents) {
        file << ev.step << " "
             << (ev.added ? "ADD" : "REMOVE") << " "
             << ev.u << " " << ev.v << " " << ev.weight << "\n";
    }
    file << "\n";
}

void writeFinalOpinions(std::ofstream& file, const SimulationLog& log) {
    file << "FINAL_OPINIONS " << log.finalOpinions.size() << "\n";
    for (int op : log.finalOpinions) {
        file << op << "\n";
    }
}

} // namespace

void saveSimulationLog(const SimulationLog& log, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("saveSimulationLog: не удалось открыть файл: " + filename);
    }

    writeParams(file, log);
    writeHistory(file, log);
    writeOpinionChanges(file, log);
    writeEdgeEvents(file, log);
    writeFinalOpinions(file, log);

    if (!file.good()) {
        throw std::runtime_error("saveSimulationLog: ошибка при записи в файл: " + filename);
    }
}

} // namespace od::io