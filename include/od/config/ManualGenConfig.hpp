#pragma once

#include "od/model/InitialOpinions.hpp"
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

    int numOpinions = od::model::DEFAULT_NUM_OPINIONS;
    od::model::OpinionFunction opinionFunction = od::model::defaultOpinionFunction();
    od::model::ChoiceRule choiceRule = od::model::ChoiceRule::Max;
    std::vector<int> popularity;
    od::model::InitialOpinionSettings initialOpinions;
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

/**
 * @brief Спрашивает число мнений, функцию изменения мнений, правило выбора, популярность,
 *        начальные мнения и учёт весов.
 * @param cnf Конфигурация, куда записываются ответы.
 * @param hasStubborn Есть ли в графе упрямые вершины: только тогда спрашивается их начальное мнение.
 */
void collectOpinionModelSettings(ManualGenConfig& cnf, bool hasStubborn);

od::model::ProbabilityFunction collectProbabilityFunction(const std::string& title);
od::model::OpinionFunction collectOpinionFunction(int numOpinions);

ManualGenConfig collectManualGenConfig();
ManualGenConfig collectGeneralGenConfig();

/// @brief Общее число вершин графа: вершины уровней и прикрепляемые упрямые.
int countGraphVertices(const ManualGenConfig& config);

ValidationResult validateConfig(const ManualGenConfig& config);

} // namespace od::config
