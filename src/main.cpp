#include <iostream>
#include <string>


#include "od/config/ManualGenConfig.hpp"
#include <iostream>

int main(){
    auto cnf = od::config::collectManualGenConfig();
    auto result = od::config::validateConfig(cnf);
    if (!result.isValid) {
        std::cout << "Ошибка: " << result.errorMessage << "\n";
    } else {
        std::cout << "Конфиг корректен\n";
        std::cout << "Уровней: " << cnf.cntLevels << "\n";
        for (size_t i = 0; i < cnf.components.size(); ++i) {
            std::cout << "  Уровень " << i + 1 << ": вершин=" << cnf.components[i].numVertices
                       << ", p=" << cnf.components[i].probability << "\n";
        }
        std::cout << "Упрямые вершины: " << (cnf.generateStubborn ? "да" : "нет");
        if (cnf.generateStubborn) std::cout << " (" << cnf.numStubbornVertices << ")";
        std::cout << "\n";
        std::cout << "Динамика рёбер: " << (cnf.dynamicEdges ? "да" : "нет");
        if (cnf.dynamicEdges) std::cout << " (p0=" << cnf.p0 << ", k=" << cnf.k << ")";
        std::cout << "\n";
    }
    return 0;
}