#include "od/graph/Graph.hpp"

#include <stdexcept>
#include <string>

namespace od::graph {

namespace {

std::size_t checkedVertexCount(int numVertices) {
    if (numVertices < 0) {
        throw std::invalid_argument("Graph: количество вершин не может быть отрицательным ("
                                    + std::to_string(numVertices) + ")");
    }
    return static_cast<std::size_t>(numVertices);
}

void checkWeight(double weight) {
    if (!(weight >= 0.0 && weight <= 1.0)) {
        throw std::invalid_argument("Graph: вес ребра должен быть в диапазоне [0, 1], получено "
                                    + std::to_string(weight));
    }
}

} // namespace

Graph::Graph(int numVertices)
    : adjacency_(checkedVertexCount(numVertices))
    , stubborn_(adjacency_.size(), 0) {
}

std::size_t Graph::index(int vertex) const {
    if (vertex < 0 || vertex >= getNumVertices()) {
        throw std::out_of_range("Graph: индекс вершины " + std::to_string(vertex)
                                + " вне диапазона [0, " + std::to_string(getNumVertices()) + ")");
    }
    return static_cast<std::size_t>(vertex);
}

void Graph::checkEdgeEnds(int from, int to) const {
    index(from);
    index(to);
    if (from == to) {
        throw std::invalid_argument("Graph: петли запрещены (вершина " + std::to_string(from) + ")");
    }
}

int Graph::addVertex() {
    adjacency_.emplace_back();
    stubborn_.push_back(0);
    return getNumVertices() - 1;
}

void Graph::addEdge(int from, int to, double weight) {
    checkEdgeEnds(from, to);
    checkWeight(weight);

    adjacency_[index(to)][from] = weight;

    if (adjacency_[index(from)].insert_or_assign(to, weight).second) {
        ++numEdges_;
    }
}

std::optional<double> Graph::removeEdge(int from, int to) {
    checkEdgeEnds(from, to);

    Neighbors& fromNeighbors = adjacency_[index(from)];
    auto iter = fromNeighbors.find(to);
    if (iter == fromNeighbors.end()) {
        return std::nullopt;
    }

    double weight = iter->second;
    fromNeighbors.erase(iter);

    adjacency_[index(to)].erase(from);
    --numEdges_;
    return weight;
}

bool Graph::hasEdge(int from, int to) const {
    return getWeight(from, to).has_value();
}

std::optional<double> Graph::getWeight(int from, int to) const {
    const Neighbors& neighbors = adjacency_[index(from)];
    index(to);

    auto iter = neighbors.find(to);
    if (iter == neighbors.end()) {
        return std::nullopt;
    }
    return iter->second;
}

const Graph::Neighbors& Graph::getNeighbors(int vertex) const {
    return adjacency_[index(vertex)];
}

void Graph::setStubborn(int vertex, bool value) {
    stubborn_[index(vertex)] = value ? 1 : 0;
}

bool Graph::isStubborn(int vertex) const {
    return stubborn_[index(vertex)] != 0;
}

} // namespace od::graph