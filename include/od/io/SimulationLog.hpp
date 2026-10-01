#pragma once

#include "od/model/EdgeEvolution.hpp"

#include <string>
#include <vector>

namespace od::io {

struct StepRecord {
    int ones;
    long long edges;
};

struct OpinionChange {
    int step;
    int vertex;
    int from;
    int to;
};

struct SimulationLog {
    unsigned int seed = 0;
    int tMax = 0;
    double k1 = 0.0;
    double k2 = 0.0;
    bool dynamicEdges = false;
    double p0 = 0.0;
    double k = 0.0;
    bool removeEdges = false;
    double removeP0 = 0.0;
    double removeK = 0.0;

    std::string initialGraphFile;
    std::string finalGraphFile;

    std::vector<StepRecord> history;
    std::vector<OpinionChange> opinionChanges;
    std::vector<od::model::EdgeEvent> edgeEvents;
    std::vector<int> finalOpinions;
};

void saveSimulationLog(const SimulationLog& log, const std::string& filename);

} // namespace od::io