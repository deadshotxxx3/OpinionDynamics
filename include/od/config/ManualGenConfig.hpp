#pragma once

#include <vector>
#include <string>

namespace od::config {

struct LevelConfig {
    int numVertices;
    double probability;
};

struct ManualGenConfig {
    int cntLevels;
    std::vector<LevelConfig> components;
    bool generateStubborn;
    int numStubbornVertices;
    bool stubbornManualAttach;
    std::vector<int> stubbornTargets;

    bool dynamicEdges;
    double p0;
    double k;

    bool removeEdges;
    double removeP0;
    double removeK;

    double k1;
    double k2;
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

ManualGenConfig collectManualGenConfig();
ManualGenConfig collectGeneralGenConfig();

ValidationResult validateConfig(const ManualGenConfig& config);

} // namespace od::config