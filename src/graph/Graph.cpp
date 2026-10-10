#include "od/graph/Graph.hpp"
#include <stdexcept>

namespace od::graph {

Graph::Graph(int numVertices)
    : numVertices_(numVertices)
    , adjacency_(numVertices < 0 ? static_cast<size_t>(0) : static_cast<size_t>(numVertices))
    , stubborn_(numVertices < 0 ? static_cast<size_t>(0) : static_cast<size_t>(numVertices), false) {
    if (numVertices < 0) {
        throw std::invalid_argument("Graph: количество вершин не может быть отрицательным");
    }
}

void Graph::addEdge(int from, int to, double weight) {
    checkVertex(from);
    checkVertex(to);
    adjacency_[static_cast<size_t>(from)][to] = weight;
    if (from != to) {
        adjacency_[static_cast<size_t>(to)][from] = weight;
    }
}

void Graph::removeEdge(int from, int to) {
    checkVertex(from);
    checkVertex(to);
    adjacency_[static_cast<size_t>(from)].erase(to);
    if (from != to) {
        adjacency_[static_cast<size_t>(to)].erase(from);
    }
}

bool Graph::hasEdge(int from, int to) const {
    checkVertex(from);
    checkVertex(to);
    const auto& neighbors = adjacency_[static_cast<size_t>(from)];
    return neighbors.find(to) != neighbors.end();
}

const std::unordered_map<int, double>& Graph::getNeighbors(int vertex) const {
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

int Graph::addVertex() {
    int index = numVertices_;
    ++numVertices_;
    adjacency_.emplace_back();
    stubborn_.push_back(false);
    return index;
}

} // namespace od::graph