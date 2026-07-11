#pragma once

#include <vector>
#include <limits>
#include "graph/grid.hpp"

namespace gridpulse {

struct SPFAResult {
    std::vector<double> distance;     // distance from source to each node
    std::vector<int> predecessor;     // predecessor node in shortest path
    std::vector<int> predecessorEdge; // edge ID used to reach this node
    bool negativeCycle;               // true if negative cycle detected
    bool success;                     // true if computation completed

    SPFAResult() : negativeCycle(false), success(false) {}
};

class SPFA {
public:
    // Run SPFA with custom edge weights
    // edgeWeights: parallel array to grid edges, can contain negative values
    // Only traverses non-tripped edges
    static SPFAResult shortestPaths(
        const Grid& grid,
        int sourceId,
        const std::vector<double>& edgeWeights
    );

    // Reconstruct path from source to target using SPFA result
    static std::vector<int> reconstructPath(
        const Grid& grid,
        const SPFAResult& result,
        int targetId
    );

    // Convenience: shortest path with custom weights
    static std::pair<double, std::vector<int>> shortestPath(
        const Grid& grid,
        int sourceId,
        int targetId,
        const std::vector<double>& edgeWeights
    );

private:
    static constexpr int MAX_ITERATIONS = 1000; // safety limit
};

} // namespace gridpulse