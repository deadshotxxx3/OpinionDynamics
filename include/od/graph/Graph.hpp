#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

namespace od::graph {

class Graph {
public:
    using Neighbors = std::unordered_map<int, double>;

    explicit Graph(int numVertices);

    int addVertex();

    void addEdge(int from, int to, double weight);
    std::optional<double> removeEdge(int from, int to);
    bool hasEdge(int from, int to) const;
    std::optional<double> getWeight(int from, int to) const;

    int getNumVertices() const noexcept {
        return static_cast<int>(adjacency_.size());
    }

    long long getNumEdges() const noexcept {
        return numEdges_;
    }

    const Neighbors& getNeighbors(int vertex) const;

    void setStubborn(int vertex, bool value);
    bool isStubborn(int vertex) const;

private:
    std::size_t index(int vertex) const;
    void checkEdgeEnds(int from, int to) const;

    std::vector<Neighbors> adjacency_;
    std::vector<std::uint8_t> stubborn_;
    long long numEdges_ = 0;
};

} // namespace od::graph