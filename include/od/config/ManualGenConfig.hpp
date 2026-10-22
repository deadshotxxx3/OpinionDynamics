#pragma once

#include "od/model/OpinionFunction.hpp"
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

    od::model::OpinionFunction opinionFunction = od::model::defaultOpinionFunction();
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
od::model::OpinionFunction collectOpinionFunction();

ManualGenConfig collectManualGenConfig();
ManualGenConfig collectGeneralGenConfig();

ValidationResult validateConfig(const ManualGenConfig& config);

} // namespace od::config
