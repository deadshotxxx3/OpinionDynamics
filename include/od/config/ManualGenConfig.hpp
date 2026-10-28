#pragma once

#include "od/model/InitialOpinions.hpp"
#include "od/model/OpinionFunction.hpp"
#include "od/model/ProbabilityFunction.hpp"

#include <string>
#include <vector>

namespace od::config {

/// @brief Один уровень графа: число вершин и вероятность ребра внутри уровня.
struct LevelConfig {
    int numVertices;
    double probability;
};

/// @brief Группа упрямых вершин: сколько и какие (случайно или по списку).
struct StubbornGroup {
    int count = 0;
    bool manual = false;     ///< `true` — вершины берутся из `targets`, иначе выбираются случайно.
    std::vector<int> targets;
};

/// @brief Всё, что нужно для генерации графа и запуска на нём симуляции.
struct ManualGenConfig {
    int cntLevels = 0;
    std::vector<LevelConfig> components;

    bool generateStubborn = false;
    StubbornGroup stubbornAssign; ///< Существующие вершины, которые становятся упрямыми.
    StubbornGroup stubbornAttach; ///< Новые упрямые вершины степени 1, прикреплённые к графу.

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

/// @brief Результат проверки: `isValid` и текст ошибки, если конфигурация некорректна.
struct ValidationResult {
    bool isValid;
    std::string errorMessage;
};

/// @brief Число вершин во всех уровнях, без прикреплённых упрямых.
int countLevelVertices(const ManualGenConfig& config);

/// @brief Сколько упрямых вершин можно прикрепить к графу из `levelVertices` вершин.
int maxAttachCount(int levelVertices);

/// @brief Общее число вершин графа: вершины уровней и прикрепляемые упрямые.
int countGraphVertices(const ManualGenConfig& config);

/// @brief Проверяет уровни, упрямые вершины, динамику рёбер и модель мнений.
ValidationResult validateConfig(const ManualGenConfig& config);

} // namespace od::config
