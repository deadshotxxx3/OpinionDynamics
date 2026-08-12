#pragma once

#include <vector>
#include <stdexcept>

namespace od::graph {

struct Edge {
    int to;
    double weight;
};

class Graph {
public:
    explicit Graph(int numVertices);

    void addEdge(int from, int to, double weight);

    int getNumVertices() const noexcept {
        return numVertices_;
    }

    const std::vector<Edge>& getNeighbors(int vertex) const;

    void setStubborn(int vertex, bool value);
    bool isStubborn(int vertex) const;

private:
    void checkVertex(int vertex) const;

    int numVertices_;
    std::vector<std::vector<Edge>> adjacency_;
    std::vector<char> stubborn_;
};

}