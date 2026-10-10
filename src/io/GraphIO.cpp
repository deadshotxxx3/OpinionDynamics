#include "od/io/GraphIO.hpp"

#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <vector>

namespace od::io {

void writeFunction(std::ostream& out, const od::model::ProbabilityFunction& function) {
    out << od::model::kindToString(function.kind) << " " << function.params.size();
    for (double value : function.params) {
        out << " " << std::setprecision(15) << value;
    }
}

od::model::ProbabilityFunction readFunction(std::istream& in, const std::string& source) {
    std::string name;
    int count = 0;
    if (!(in >> name >> count) || count < 0) {
        throw std::runtime_error("Не удалось прочитать функцию в " + source);
    }

    auto kind = od::model::kindFromString(name);
    if (!kind.has_value()) {
        throw std::runtime_error("Неизвестная функция '" + name + "' в " + source);
    }

    od::model::ProbabilityFunction function;
    function.kind = *kind;
    function.params.assign(static_cast<size_t>(count), 0.0);
    for (int i = 0; i < count; ++i) {
        if (!(in >> function.params[static_cast<size_t>(i)])) {
            throw std::runtime_error("Не удалось прочитать параметры функции в " + source);
        }
    }

    std::string error = od::model::validateFunction(function);
    if (!error.empty()) {
        throw std::runtime_error(error + " (" + source + ")");
    }
    return function;
}

void saveGraph(const od::graph::Graph& graph, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("saveGraph: не удалось открыть файл для записи: " + filename);
    }

    int numVertices = graph.getNumVertices();
    file << "VERTICES " << numVertices << "\n";

    std::vector<int> stubbornVertices;
    for (int v = 0; v < numVertices; ++v) {
        if (graph.isStubborn(v)) {
            stubbornVertices.push_back(v);
        }
    }

    file << "STUBBORN " << stubbornVertices.size() << "\n";
    for (int v : stubbornVertices) {
        file << v << "\n";
    }

    struct EdgeRecord {
        int from;
        int to;
        double weight;
    };
    std::vector<EdgeRecord> edges;

    for (int u = 0; u < numVertices; ++u) {
        for (const auto& [neighborId, weight] : graph.getNeighbors(u)) {
            if (u < neighborId) {
                edges.push_back(EdgeRecord{u, neighborId, weight});
            }
        }
    }

    file << "EDGES " << edges.size() << "\n";
    for (const auto& e : edges) {
        file << e.from << " " << e.to << " " << e.weight << "\n";
    }

    if (!file.good()) {
        throw std::runtime_error("saveGraph: ошибка при записи в файл: " + filename);
    }
}

namespace {

void expectToken(std::ifstream& file, const std::string& expected, const std::string& filename) {
    std::string token;
    if (!(file >> token)) {
        throw std::runtime_error("loadGraph: неожиданный конец файла " + filename +
                                 ", ожидался токен '" + expected + "'");
    }
    if (token != expected) {
        throw std::runtime_error("loadGraph: в файле " + filename +
                                 " ожидался токен '" + expected + "', получен '" + token + "'");
    }
}

int readCount(std::ifstream& file, const std::string& what, const std::string& filename) {
    int value = 0;
    if (!(file >> value)) {
        throw std::runtime_error("loadGraph: не удалось прочитать " + what + " из " + filename);
    }
    if (value < 0) {
        throw std::runtime_error("loadGraph: " + what + " отрицательное в " + filename);
    }
    return value;
}

struct LegacyLinear {
    bool hasA = false;
    bool hasB = false;
    double a = 0.0;
    double b = 0.0;

    bool present() const {
        return hasA || hasB;
    }

    od::model::ProbabilityFunction toFunction() const {
        od::model::ProbabilityFunction function;
        function.kind = od::model::FunctionKind::Linear;
        function.params = {a, b};
        return function;
    }
};

void readParamsSection(std::ifstream& file, SimulationParams& params, const std::string& filename) {
    bool hasAddFunction = false;
    bool hasRemoveFunction = false;
    LegacyLinear legacyAdd;
    LegacyLinear legacyRemove;

    std::string token;
    while (file >> token) {
        if (token == "SEED") {
            file >> params.seed;
        } else if (token == "T_MAX") {
            file >> params.tMax;
        } else if (token == "K1") {
            file >> params.k1;
        } else if (token == "K2") {
            file >> params.k2;
        } else if (token == "USE_WEIGHTS") {
            int v = 0;
            file >> v;
            params.useWeights = (v != 0);
        } else if (token == "DYNAMIC_EDGES") {
            int v = 0;
            file >> v;
            params.dynamicEdges = (v != 0);
        } else if (token == "REMOVE_EDGES") {
            int v = 0;
            file >> v;
            params.removeEdges = (v != 0);
        } else if (token == "ADD_FUNCTION") {
            params.edgeAddFunction = readFunction(file, filename);
            hasAddFunction = true;
        } else if (token == "REMOVE_FUNCTION") {
            params.edgeRemoveFunction = readFunction(file, filename);
            hasRemoveFunction = true;
        } else if (token == "P0") {
            file >> legacyAdd.a;
            legacyAdd.hasA = true;
        } else if (token == "K") {
            file >> legacyAdd.b;
            legacyAdd.hasB = true;
        } else if (token == "REMOVE_P0") {
            file >> legacyRemove.a;
            legacyRemove.hasA = true;
        } else if (token == "REMOVE_K") {
            file >> legacyRemove.b;
            legacyRemove.hasB = true;
        }
    }

    if (!hasAddFunction && legacyAdd.present()) {
        params.edgeAddFunction = legacyAdd.toFunction();
    }
    if (!hasRemoveFunction && legacyRemove.present()) {
        params.edgeRemoveFunction = legacyRemove.toFunction();
    }
}

} // namespace

od::graph::Graph loadGraph(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("loadGraph: не удалось открыть файл: " + filename);
    }

    expectToken(file, "VERTICES", filename);
    int numVertices = readCount(file, "количество вершин", filename);

    od::graph::Graph graph(numVertices);

    expectToken(file, "STUBBORN", filename);
    int stubbornCount = readCount(file, "количество упрямых вершин", filename);
    if (stubbornCount > numVertices) {
        throw std::runtime_error("loadGraph: некорректное количество упрямых вершин в " + filename);
    }

    for (int i = 0; i < stubbornCount; ++i) {
        int idx = 0;
        if (!(file >> idx)) {
            throw std::runtime_error("loadGraph: не удалось прочитать индекс упрямой вершины из " + filename);
        }
        if (idx < 0 || idx >= numVertices) {
            throw std::runtime_error("loadGraph: индекс упрямой вершины вне диапазона в " + filename);
        }
        graph.setStubborn(idx, true);
    }

    expectToken(file, "EDGES", filename);
    int edgeCount = readCount(file, "количество рёбер", filename);

    for (int i = 0; i < edgeCount; ++i) {
        int from = 0;
        int to = 0;
        double weight = 0.0;
        if (!(file >> from >> to >> weight)) {
            throw std::runtime_error("loadGraph: не удалось прочитать ребро из " + filename);
        }
        if (from < 0 || from >= numVertices || to < 0 || to >= numVertices) {
            throw std::runtime_error("loadGraph: индексы ребра вне диапазона в " + filename);
        }
        graph.addEdge(from, to, weight);
    }

    return graph;
}

void saveGraphWithParams(
    const od::graph::Graph& graph,
    const SimulationParams& params,
    const std::string& filename
) {
    saveGraph(graph, filename);

    std::ofstream file(filename, std::ios::app);
    if (!file.is_open()) {
        throw std::runtime_error("saveGraphWithParams: не удалось открыть файл для дозаписи: " + filename);
    }

    file << "\n";
    file << "PARAMS\n";
    file << "SEED " << params.seed << "\n";
    file << "T_MAX " << params.tMax << "\n";
    file << "K1 " << std::setprecision(15) << params.k1 << "\n";
    file << "K2 " << std::setprecision(15) << params.k2 << "\n";
    file << "USE_WEIGHTS " << (params.useWeights ? 1 : 0) << "\n";
    file << "DYNAMIC_EDGES " << (params.dynamicEdges ? 1 : 0) << "\n";
    file << "ADD_FUNCTION ";
    writeFunction(file, params.edgeAddFunction);
    file << "\n";
    file << "REMOVE_EDGES " << (params.removeEdges ? 1 : 0) << "\n";
    file << "REMOVE_FUNCTION ";
    writeFunction(file, params.edgeRemoveFunction);
    file << "\n";

    if (!file.good()) {
        throw std::runtime_error("saveGraphWithParams: ошибка при записи PARAMS в " + filename);
    }
}

od::graph::Graph loadGraphWithParams(
    const std::string& filename,
    SimulationParams& params,
    bool& paramsLoaded
) {
    paramsLoaded = false;

    od::graph::Graph graph = loadGraph(filename);

    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("loadGraphWithParams: не удалось открыть файл: " + filename);
    }

    std::string token;
    while (file >> token) {
        if (token == "PARAMS") {
            paramsLoaded = true;
            break;
        }
    }

    if (paramsLoaded) {
        readParamsSection(file, params, filename);
    }

    return graph;
}

} // namespace od::io