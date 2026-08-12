#include "od/graph/Graph.hpp"
#include <stdexcept>

namespace od::graph {

Graph::Graph(int numVertices)
    : numVertices_(numVertices)
    , adjacency_(numVertices < 0 ? static_cast<size_t>(0)
                                 : static_cast<size_t>(numVertices))
    , stubborn_(numVertices < 0 ? static_cast<size_t>(0)
                                : static_cast<size_t>(numVertices), false) {
    if (numVertices < 0) {
        throw std::invalid_argument("Graph: количество вершин не может быть отрицательным");
    }
}

void Graph::addEdge(int from, int to, double weight) {
    checkVertex(from);
    checkVertex(to);

    adjacency_[static_cast<size_t>(from)].push_back(Edge{to, weight});

    if (from != to) {
        adjacency_[static_cast<size_t>(to)].push_back(Edge{from, weight});
    }
}

const std::vector<Edge>& Graph::getNeighbors(int vertex) const {
    checkVertex(vertex);

    return adjacency_[static_cast<size_t>(vertex)];
}

void Graph::setStubborn(int vertex, bool value) {
    checkVertex(vertex);

    stubborn_[static_cast<size_t>(vertex)] = static_cast<char>(value);
}

bool Graph::isStubborn(int vertex) const {
    checkVertex(vertex);

    return stubborn_[static_cast<size_t>(vertex)];
}

void Graph::checkVertex(int vertex) const {
    if (vertex < 0 || vertex >= numVertices_) {
        throw std::out_of_range("Graph: индекс вершины out of range");
    }
}

}