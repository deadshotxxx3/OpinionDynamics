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
    bool dynamicEdges;
    double p0;
    double k;
    double k1;
    double k2;
};
struct ValidationResult {
    bool isValid;
    std::string errorMessage;
};

void collectLevels(ManualGenConfig& cnf);   
void collectStubbornSettings(ManualGenConfig& cnf);
void collectDynamicEdgesSettings(ManualGenConfig& cnf);
void collectOpinionModelSettings(ManualGenConfig& cnf);

ManualGenConfig collectManualGenConfig();
ValidationResult validateConfig(const ManualGenConfig& config);
} 