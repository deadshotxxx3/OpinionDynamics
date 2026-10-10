#pragma once

#include "od/model/ProbabilityFunction.hpp"

#include <string>
#include <vector>

namespace od::config {

struct LevelConfig {
    int numVertices;
    double probability;
};

struct StubbornGroup {
    int count = 0;
    bool manual = false;
    std::vector<int> targets;
};

struct ManualGenConfig {
    int cntLevels = 0;
    std::vector<LevelConfig> components;

    bool generateStubborn = false;
    StubbornGroup stubbornAssign;
    StubbornGroup stubbornAttach;

    bool dynamicEdges = false;
    od::model::ProbabilityFunction edgeAddFunction;

    bool removeEdges = false;
    od::model::ProbabilityFunction edgeRemoveFunction;

    double k1 = 0.0;
    double k2 = 1.0;
    bool useWeights = false;
};

struct ValidationResult {
    bool isValid;
    std::string errorMessage;
};

enum class GenerationMode {
    Manual,
    General
};

GenerationMode chooseGenerationMode();

void collectLevels(ManualGenConfig& cnf);
void collectStubbornSettings(ManualGenConfig& cnf);
void collectDynamicEdgesSettings(ManualGenConfig& cnf);
void collectOpinionModelSettings(ManualGenConfig& cnf);

od::model::ProbabilityFunction collectProbabilityFunction(const std::string& title);

ManualGenConfig collectManualGenConfig();
ManualGenConfig collectGeneralGenConfig();

ValidationResult validateConfig(const ManualGenConfig& config);

} // namespace od::config