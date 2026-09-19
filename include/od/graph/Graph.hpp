#pragma once

#include <vector>
#include <unordered_map>
#include <stdexcept>

namespace od::graph {

class Graph {
public:
    explicit Graph(int numVertices);

    void addEdge(int from, int to, double weight);
    int addVertex();
    bool hasEdge(int from, int to) const;

    int getNumVertices() const noexcept {
        return numVertices_;
    }

    const std::unordered_map<int, double>& getNeighbors(int vertex) const;

    void setStubborn(int vertex, bool value);
    bool isStubborn(int vertex) const;

private:
    void checkVertex(int vertex) const;

    int numVertices_;
    std::vector<std::unordered_map<int, double>> adjacency_;
    std::vector<char> stubborn_;
};

} // namespace od::graph