#include "od/config/ManualGenConfig.hpp"

#include "od/config/Limits.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace od::config {

namespace {

ValidationResult ok() {
    return {true, ""};
}

ValidationResult fail(const std::string& message) {
    return {false, message};
}

ValidationResult validateLevels(const ManualGenConfig& config) {
    if (config.cntLevels < 1) {
        return fail("Количество уровней должно быть не меньше 1");
    }
    if (static_cast<int>(config.components.size()) != config.cntLevels) {
        return fail("Количество введённых уровней не совпадает с cntLevels");
    }
    for (const LevelConfig& level : config.components) {
        if (level.numVertices <= 0) {
            return fail("Количество вершин в уровне должно быть больше 0");
        }
        if (level.probability < 0.0 || level.probability > 1.0) {
            return fail("Вероятность уровня должна быть в диапазоне [0.0, 1.0]");
        }
    }
    if (countLevelVertices(config) > MAX_VERTICES) {
        return fail("Суммарное количество вершин превышает " + std::to_string(MAX_VERTICES));
    }
    return ok();
}

ValidationResult validateStubbornGroup(const StubbornGroup& group, const std::string& title,
                                       int maxCount, int levelVertices, bool unique) {
    if (group.count < 0 || group.count > maxCount) {
        return fail(title + ": количество должно быть от 0 до " + std::to_string(maxCount));
    }
    if (!group.manual || group.count == 0) {
        return ok();
    }
    if (static_cast<int>(group.targets.size()) != group.count) {
        return fail(title + ": число выбранных вершин не совпадает с количеством");
    }

    std::vector<char> used(static_cast<size_t>(levelVertices), 0);
    for (int vertex : group.targets) {
        if (vertex < 0 || vertex >= levelVertices) {
            return fail(title + ": индекс вершины вне диапазона");
        }
        if (unique && used[static_cast<size_t>(vertex)]) {
            return fail(title + ": вершина выбрана дважды");
        }
        used[static_cast<size_t>(vertex)] = 1;
    }
    return ok();
}

ValidationResult validateStubborn(const ManualGenConfig& config) {
    if (!config.generateStubborn) {
        return ok();
    }

    const int levelVertices = countLevelVertices(config);
    ValidationResult result = validateStubbornGroup(
        config.stubbornAssign, "Назначение существующих", levelVertices, levelVertices, true);
    if (!result.isValid) {
        return result;
    }
    result = validateStubbornGroup(
        config.stubbornAttach, "Прикрепление новых", maxAttachCount(levelVertices), levelVertices, false);
    if (!result.isValid) {
        return result;
    }
    if (config.stubbornAssign.count == 0 && config.stubbornAttach.count == 0) {
        return fail("Включены упрямые вершины, но не выбрано ни одной");
    }
    return ok();
}

ValidationResult validateDynamicEdges(const ManualGenConfig& config) {
    if (!config.dynamicEdges) {
        return ok();
    }
    std::string error = od::model::validateFunction(config.edgeAddFunction);
    if (!error.empty()) {
        return fail("Функция появления рёбер: " + error);
    }
    if (config.removeEdges) {
        error = od::model::validateFunction(config.edgeRemoveFunction);
        if (!error.empty()) {
            return fail("Функция исчезновения рёбер: " + error);
        }
    }
    return ok();
}

ValidationResult validateOpinionModel(const ManualGenConfig& config) {
    if (config.numOpinions < 2 || config.numOpinions > od::model::MAX_OPINIONS) {
        return fail("Число мнений должно быть от 2 до " + std::to_string(od::model::MAX_OPINIONS));
    }
    std::string error = od::model::validateOpinionSettings(
        config.opinionFunction, config.popularity, config.numOpinions);
    if (error.empty()) {
        error = od::model::validateInitialSettings(config.initialOpinions, config.numOpinions);
    }
    if (!error.empty()) {
        return fail("Модель мнений: " + error);
    }

    const int totalVertices = countGraphVertices(config);
    for (const od::model::ManualOpinion& item : config.initialOpinions.manual) {
        if (item.vertex >= totalVertices) {
            return fail("Начальные мнения: вершины " + std::to_string(item.vertex)
                        + " нет в графе, допустимые номера от 0 до " + std::to_string(totalVertices - 1));
        }
    }
    return ok();
}

} // namespace

int countLevelVertices(const ManualGenConfig& config) {
    int total = 0;
    for (const LevelConfig& level : config.components) {
        total += level.numVertices;
    }
    return total;
}

int maxAttachCount(int levelVertices) {
    return std::min(levelVertices, MAX_VERTICES - levelVertices);
}

int countGraphVertices(const ManualGenConfig& config) {
    const int total = countLevelVertices(config);
    return config.generateStubborn ? total + config.stubbornAttach.count : total;
}

ValidationResult validateConfig(const ManualGenConfig& config) {
    for (auto check : {validateLevels, validateStubborn, validateDynamicEdges, validateOpinionModel}) {
        ValidationResult result = check(config);
        if (!result.isValid) {
            return result;
        }
    }
    return ok();
}

} // namespace od::config
