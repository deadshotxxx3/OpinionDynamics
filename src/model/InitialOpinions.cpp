#include "od/model/InitialOpinions.hpp"

#include "od/io/InputHelpers.hpp"

#include <random>
#include <set>
#include <sstream>
#include <stdexcept>

namespace od::model {

namespace {

constexpr unsigned int INITIAL_OPINIONS_RANDOM_STREAM = 2;

std::mt19937 makeInitialRng(unsigned int seed) {
    std::seed_seq sequence{seed, INITIAL_OPINIONS_RANDOM_STREAM};
    return std::mt19937(sequence);
}

std::string checkOpinion(int opinion, int numOpinions, const std::string& what) {
    if (opinion < 0 || opinion >= numOpinions) {
        return what + ": мнение " + std::to_string(opinion) + " вне диапазона от 0 до "
               + std::to_string(numOpinions - 1);
    }
    return "";
}

std::string checkGroup(const InitialGroup& group, int numOpinions, const std::string& what) {
    return group.random ? "" : checkOpinion(group.value, numOpinions, what);
}

} // namespace

std::vector<int> makeInitialOpinions(const od::graph::Graph& graph, const InitialOpinionSettings& settings,
                                     int numOpinions, unsigned int seed) {
    const int n = graph.getNumVertices();
    std::vector<int> opinions(static_cast<size_t>(n), 0);

    std::mt19937 rng = makeInitialRng(seed);
    std::uniform_int_distribution<int> randomOpinion(0, numOpinions - 1);

    for (int v = 0; v < n; ++v) {
        const InitialGroup& group = graph.isStubborn(v) ? settings.stubborn : settings.regular;
        opinions[static_cast<size_t>(v)] = group.random ? randomOpinion(rng) : group.value;
    }

    for (const ManualOpinion& item : settings.manual) {
        if (item.vertex < 0 || item.vertex >= n) {
            throw std::invalid_argument("начальные мнения: вершины " + std::to_string(item.vertex)
                                        + " нет в графе, допустимые номера от 0 до " + std::to_string(n - 1));
        }
        opinions[static_cast<size_t>(item.vertex)] = item.opinion;
    }
    return opinions;
}

std::string validateInitialSettings(const InitialOpinionSettings& settings, int numOpinions) {
    std::string error = checkGroup(settings.regular, numOpinions, "начальное мнение обычных вершин");
    if (error.empty()) {
        error = checkGroup(settings.stubborn, numOpinions, "начальное мнение упрямых вершин");
    }
    if (!error.empty()) {
        return error;
    }

    std::set<int> vertices;
    for (const ManualOpinion& item : settings.manual) {
        if (item.vertex < 0) {
            return "начальные мнения: номер вершины не может быть отрицательным";
        }
        if (!vertices.insert(item.vertex).second) {
            return "начальные мнения: вершина " + std::to_string(item.vertex) + " указана дважды";
        }
        error = checkOpinion(item.opinion, numOpinions, "начальное мнение вершины " + std::to_string(item.vertex));
        if (!error.empty()) {
            return error;
        }
    }
    return "";
}

std::string validateOpinionVector(const std::vector<int>& opinions, int numVertices, int numOpinions) {
    if (static_cast<int>(opinions.size()) != numVertices) {
        return "мнений " + std::to_string(opinions.size()) + ", а вершин " + std::to_string(numVertices);
    }
    for (size_t v = 0; v < opinions.size(); ++v) {
        std::string error = checkOpinion(opinions[v], numOpinions, "вершина " + std::to_string(v));
        if (!error.empty()) {
            return error;
        }
    }
    return "";
}

std::string initialGroupToString(const InitialGroup& group) {
    return group.random ? "random" : std::to_string(group.value);
}

std::optional<InitialGroup> initialGroupFromString(const std::string& text) {
    const std::string value = od::io::trim(text);
    if (value == "random") {
        return InitialGroup{true, 0};
    }
    std::optional<int> opinion = od::io::parseInt(value);
    if (!opinion.has_value() || *opinion < 0) {
        return std::nullopt;
    }
    return InitialGroup{false, *opinion};
}

std::string manualOpinionsToString(const std::vector<ManualOpinion>& manual) {
    std::ostringstream out;
    for (size_t i = 0; i < manual.size(); ++i) {
        if (i > 0) out << ",";
        out << manual[i].vertex << ":" << manual[i].opinion;
    }
    return out.str();
}

std::optional<std::vector<ManualOpinion>> manualOpinionsFromString(const std::string& text) {
    std::vector<ManualOpinion> manual;
    if (od::io::trim(text).empty()) {
        return manual;
    }

    std::stringstream stream(text);
    std::string item;
    while (std::getline(stream, item, ',')) {
        const size_t colon = item.find(':');
        if (colon == std::string::npos) {
            return std::nullopt;
        }
        std::optional<int> vertex = od::io::parseInt(item.substr(0, colon));
        std::optional<int> opinion = od::io::parseInt(item.substr(colon + 1));
        if (!vertex.has_value() || !opinion.has_value()) {
            return std::nullopt;
        }
        manual.push_back(ManualOpinion{*vertex, *opinion});
    }
    return manual;
}

std::string describeInitialGroup(const InitialGroup& group) {
    return group.random ? "случайно" : "мнение " + std::to_string(group.value);
}

} // namespace od::model
