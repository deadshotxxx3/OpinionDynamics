#include "od/io/SimulationLog.hpp"

#include <fstream>
#include <iomanip>
#include <limits>
#include <stdexcept>

namespace od::io {

namespace {

void writeParams(std::ofstream& file, const SimulationLog& log) {
    writeParamsLines(file, log.params);
    file << "\n";
    file << "INITIAL_GRAPH_FILE " << log.initialGraphFile << "\n";
    file << "FINAL_GRAPH_FILE " << log.finalGraphFile << "\n\n";
}

void writeHistory(std::ofstream& file, const SimulationLog& log) {
    file << "OPINION_HISTORY " << log.history.size() << "\n";
    for (const StepRecord& record : log.history) {
        file << record.edges << " " << record.counts.size();
        for (const od::model::OpinionCount& item : record.counts) {
            file << " " << item.opinion << " " << item.count;
        }
        file << "\n";
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
    file << std::setprecision(std::numeric_limits<double>::max_digits10);
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
