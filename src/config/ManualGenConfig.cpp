#include "od/config/ManualGenConfig.hpp"
#include "od/io/InputHelpers.hpp"
#include <algorithm>
#include <iostream>
#include <string>

const int MAX_VERTICES = 10000;
const int MAX_MANUAL_ATTACH = 100;

namespace od::config {

static void collectLevelDetails(ManualGenConfig& cnf){
    int cnt_io_comp = 0;
    int correct_vertex = MAX_VERTICES;

    while (cnt_io_comp != cnf.cntLevels){
        int remainingComponents = cnf.cntLevels - cnt_io_comp;
        if (correct_vertex  < remainingComponents){
            cnt_io_comp = 0;
            cnf.components.clear();
            correct_vertex = MAX_VERTICES;
            std::cout << "Не удалось распределить вершины, начинаем ввод компонент заново\n";
            continue;
        }

        LevelConfig comp;

        std::cout << "Доступное кол-во вершин: " << correct_vertex << "\n";

        std::string prompt = "Введите кол-во вершин для " + std::to_string(cnt_io_comp + 1) + " уровня: ";

        auto val_int = od::io::readIntInRange(prompt, 1, correct_vertex);
        if (!val_int.has_value()) continue;

        comp.numVertices = *val_int;

        auto val_double = od::io::readDoubleInRange("Введите вероятность появления ребра: ", 0.0, 1.0);
        if (!val_double.has_value()) continue;

        comp.probability = *val_double;
        correct_vertex -= comp.numVertices;
        cnf.components.push_back(comp);
        cnt_io_comp++;
    }
}

void collectLevels(ManualGenConfig& cnf){
    while (true){
        auto val = od::io::readIntInRange("Введите кол-во уровней от 1 до 10 000: ", 1, 10000);

        if (!val.has_value()) continue;

        cnf.cntLevels = *val;
        break;
    }

    collectLevelDetails(cnf);
}

void collectStubbornSettings(ManualGenConfig& cnf){
    cnf.generateStubborn = od::io::readYesNo("Нужны ли упрямые вершины. Введите y/n: ");

    if (!cnf.generateStubborn){
        cnf.numStubbornVertices = 0;
        cnf.stubbornManualAttach = false;
        cnf.stubbornTargets.clear();
        return;
    }

    int totalVertices = 0;
    for (const auto& item : cnf.components) totalVertices += item.numVertices;

    while (true) {
        auto val = od::io::readIntInRange(
            "Введите кол-во упрямых вершин от 1 до " + std::to_string(totalVertices) + ": ",
            1, totalVertices);
        if (!val.has_value()) continue;
        cnf.numStubbornVertices = *val;
        break;
    }

    cnf.stubbornManualAttach = od::io::readYesNo(
        "Прикрепить упрямые вручную или случайно? (y - вручную, n - случайно): ");

    if (cnf.stubbornManualAttach && cnf.numStubbornVertices > MAX_MANUAL_ATTACH){
        std::cout << "Ручное прикрепление ограничено " << MAX_MANUAL_ATTACH
                  << " вершинами. Для " << cnf.numStubbornVertices
                  << " будет использовано случайное прикрепление.\n";
        cnf.stubbornManualAttach = false;
    }

    cnf.stubbornTargets.clear();
    if (cnf.stubbornManualAttach){
        cnf.stubbornTargets.reserve(static_cast<size_t>(cnf.numStubbornVertices));
        for (int i = 0; i < cnf.numStubbornVertices; ++i){
            std::string prompt = "К какой вершине прикрепить упрямую #"
                                 + std::to_string(i + 1) + " (0.."
                                 + std::to_string(totalVertices - 1) + "): ";
            while (true){
                auto val = od::io::readIntInRange(prompt, 0, totalVertices - 1);
                if (!val.has_value()) continue;
                cnf.stubbornTargets.push_back(*val);
                break;
            }
        }
    }
}

void collectDynamicEdgesSettings(ManualGenConfig& cnf){
    cnf.dynamicEdges = od::io::readYesNo(
        "Нужна ли динамика рёбер (изменение вероятности связи со временем)? Введите y/n: ");

    if (cnf.dynamicEdges){
        while (true) {
            auto val = od::io::readDoubleInRange(
                "Введите начальную вероятность добавления ребра (0.0..1.0): ", 0.0, 1.0);
            if (!val.has_value()) continue;
            cnf.p0 = *val;
            break;
        }
        while (true) {
            auto val = od::io::readDoubleInRange(
                "Введите коэффициент изменения для добавления (-5.0..5.0): ", -5.0, 5.0);
            if (!val.has_value()) continue;
            cnf.k = *val;
            break;
        }

        cnf.removeEdges = od::io::readYesNo("Нужно ли удаление рёбер? Введите y/n: ");
        if (cnf.removeEdges) {
            while (true) {
                auto val = od::io::readDoubleInRange(
                    "Введите начальную вероятность удаления ребра (0.0..1.0): ", 0.0, 1.0);
                if (!val.has_value()) continue;
                cnf.removeP0 = *val;
                break;
            }
            while (true) {
                auto val = od::io::readDoubleInRange(
                    "Введите коэффициент изменения для удаления (-5.0..5.0): ", -5.0, 5.0);
                if (!val.has_value()) continue;
                cnf.removeK = *val;
                break;
            }
        } else {
            cnf.removeP0 = 0.0;
            cnf.removeK = 0.0;
        }
    } else {
        cnf.p0 = 0.0;
        cnf.k = 0.0;
        cnf.removeEdges = false;
        cnf.removeP0 = 0.0;
        cnf.removeK = 0.0;
    }
}

void collectOpinionModelSettings(ManualGenConfig& cnf){
    while (true) {
        auto val = od::io::readDoubleInRange("Введите порог k1 (0.0..1.0): ", 0.0, 1.0);
        if (!val.has_value()) continue;
        cnf.k1 = *val;
        break;
    }

    while (true) {
        auto val = od::io::readDoubleInRange(
            "Введите порог k2 (" + std::to_string(cnf.k1) + "..1.0): ",
            cnf.k1, 1.0);
        if (!val.has_value()) continue;
        cnf.k2 = *val;
        break;
    }
}

GenerationMode chooseGenerationMode(){
    while (true) {
        auto val = od::io::readIntInRange(
            "Выберите способ генерации:\n"
            "1 - Ручная (свои параметры для каждого уровня)\n"
            "2 - Общая (одни параметры на все уровни)\n"
            "Ввод: ", 1, 2);
        if (!val.has_value()) continue;
        return (*val == 1) ? GenerationMode::Manual : GenerationMode::General;
    }
}

ManualGenConfig collectGeneralGenConfig(){
    ManualGenConfig cnf;

    while (true) {
        auto val = od::io::readIntInRange("Введите кол-во уровней от 1 до 10 000: ", 1, 10000);
        if (!val.has_value()) continue;
        cnf.cntLevels = *val;
        break;
    }

    int maxPerLevel = MAX_VERTICES / cnf.cntLevels;
    int verticesPerLevel = 0;
    while (true) {
        auto val = od::io::readIntInRange(
            "Введите кол-во вершин на каждый уровень (1.." + std::to_string(maxPerLevel) + "): ",
            1, maxPerLevel);
        if (!val.has_value()) continue;
        verticesPerLevel = *val;
        break;
    }

    double probability = 0.0;
    while (true) {
        auto val = od::io::readDoubleInRange("Введите вероятность появления ребра (0.0..1.0): ", 0.0, 1.0);
        if (!val.has_value()) continue;
        probability = *val;
        break;
    }

    cnf.components.clear();
    cnf.components.reserve(static_cast<size_t>(cnf.cntLevels));
    for (int i = 0; i < cnf.cntLevels; ++i) {
        cnf.components.push_back(LevelConfig{verticesPerLevel, probability});
    }

    collectStubbornSettings(cnf);
    collectDynamicEdgesSettings(cnf);
    collectOpinionModelSettings(cnf);

    return cnf;
}

ManualGenConfig collectManualGenConfig(){
    ManualGenConfig cnf;
    collectLevels(cnf);
    collectStubbornSettings(cnf);
    collectDynamicEdgesSettings(cnf);
    collectOpinionModelSettings(cnf);
    return cnf;
}

ValidationResult validateConfig(const ManualGenConfig& config){
    if (config.cntLevels < 1){
        return {false, "Количество уровней должно быть не меньше 1"};
    }

    if (static_cast<int>(config.components.size()) != config.cntLevels){
        return {false, "Количество введённых уровней не совпадает с cntLevels"};
    }

    int totalVertices = 0;
    for (const auto& level : config.components){
        if (level.numVertices <= 0){
            return {false, "Количество вершин в уровне должно быть больше 0"};
        }
        if (level.probability < 0.0 || level.probability > 1.0){
            return {false, "Вероятность уровня должна быть в диапазоне [0.0, 1.0]"};
        }
        totalVertices += level.numVertices;
    }

    if (totalVertices > MAX_VERTICES){
        return {false, "Суммарное количество вершин превышает 10000"};
    }

    if (config.generateStubborn){
        if (config.numStubbornVertices < 1 || config.numStubbornVertices > totalVertices){
            return {false, "Количество упрямых вершин должно быть от 1 до общего числа вершин"};
        }
        if (config.stubbornManualAttach){
            if (static_cast<int>(config.stubbornTargets.size()) != config.numStubbornVertices){
                return {false, "Число целей для упрямых вершин не совпадает с их количеством"};
            }
            for (int idx : config.stubbornTargets){
                if (idx < 0 || idx >= totalVertices){
                    return {false, "Индекс цели для упрямой вершины вне диапазона"};
                }
            }
        }
    }

    if (config.dynamicEdges){
        if (config.p0 < 0.0 || config.p0 > 1.0){
            return {false, "Начальная вероятность добавления p0 должна быть в диапазоне [0.0, 1.0]"};
        }
        if (config.k < -5.0 || config.k > 5.0){
            return {false, "Коэффициент добавления k должен быть в диапазоне [-5.0, 5.0]"};
        }
        if (config.removeEdges){
            if (config.removeP0 < 0.0 || config.removeP0 > 1.0){
                return {false, "Начальная вероятность удаления removeP0 должна быть в диапазоне [0.0, 1.0]"};
            }
            if (config.removeK < -5.0 || config.removeK > 5.0){
                return {false, "Коэффициент удаления removeK должен быть в диапазоне [-5.0, 5.0]"};
            }
        }
    }

    if (config.k1 < 0.0 || config.k1 > 1.0 || config.k2 < 0.0 || config.k2 > 1.0) {
        return {false, "k1 и k2 должны быть в диапазоне [0.0, 1.0]"};
    }
    if (config.k1 >= config.k2) {
        return {false, "k1 должен быть меньше k2"};
    }

    return {true, ""};
}

} // namespace od::config