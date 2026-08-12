#include "od/config/ManualGenConfig.hpp"
#include "od/io/InputHelpers.hpp"
#include <iostream>
#include <string>

const int MAX_VERTICAL = 10000;

namespace od::config{

static void collectLevelDetails(ManualGenConfig& cnf){
    int cnt_io_comp = 0;
    int correct_vertex = MAX_VERTICAL;

    while (cnt_io_comp != cnf.cntLevels){
        int remainingComponents = cnf.cntLevels - cnt_io_comp;
        if (correct_vertex  < remainingComponents){
            cnt_io_comp = 0;
            cnf.components.clear();
            correct_vertex = MAX_VERTICAL;
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
    if (cnf.generateStubborn){
        int cntComp = 0;
        for (const auto& item : cnf.components) cntComp += item.numVertices;
        while (true) {
            auto val = od::io::readIntInRange("Введите кол-во упрямых вершин от 1 до " + std::to_string(cntComp) + ": ", 1, cntComp);
            if (!val.has_value()) continue;
            cnf.numStubbornVertices = *val;
            break;
        }
    }else cnf.numStubbornVertices = 0;
}

void collectDynamicEdgesSettings(ManualGenConfig& cnf){
    cnf.dynamicEdges = od::io::readYesNo("Нужна ли динамика рёбер (изменение вероятности связи со временем)? Введите y/n: ");
    if (cnf.dynamicEdges){
        while (true) {
            auto val = od::io::readDoubleInRange("Введите начальную вероятность от 0.0 до 1.0: ", 0.0, 1.0);
            if (!val.has_value()) continue;
            cnf.p0 = *val;
            break;
        }
        while (true) {
            auto val = od::io::readDoubleInRange("Введите коэффициент изменения от -5.0 до 5.0: ", -5.0, 5.0);
            if (!val.has_value()) continue;
            cnf.k = *val;
            break;
        }
    }else {cnf.p0 = 0.0; cnf.k = 0;}
}

ManualGenConfig collectManualGenConfig(){
    ManualGenConfig cnf;
    collectLevels(cnf);
    collectStubbornSettings(cnf);
    collectDynamicEdgesSettings(cnf);
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

    if (totalVertices > 10000){
        return {false, "Суммарное количество вершин превышает 10000"};
    }

    if (config.generateStubborn){
        if (config.numStubbornVertices < 1 || config.numStubbornVertices > totalVertices){
            return {false, "Количество упрямых вершин должно быть от 1 до общего числа вершин"};
        }
    }

    if (config.dynamicEdges){
        if (config.p0 < 0.0 || config.p0 > 1.0){
            return {false, "Начальная вероятность p0 должна быть в диапазоне [0.0, 1.0]"};
        }
        if (config.k < -5.0 || config.k > 5.0){
            return {false, "Коэффициент k должен быть в диапазоне [-5.0, 5.0]"};
        }
    }

    return {true, ""};
}

}