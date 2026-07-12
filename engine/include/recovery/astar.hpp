#pragma once

#include <vector>
#include <limits>
#include "graph/grid.hpp"

namespace gridpulse {

struct AStarResult {
    std::vector<double> distance;
    std::vector<int> predecessor;
    std::vector<int> predecessorEdge;
    bool pathFound;

    AStarResult() : pathFound(false) {}
};

class AStar {
public:
    // Find shortest path using A* with straight-line distance heuristic
    static AStarResult findPath(
        const Grid& grid,
        int sourceId,
        int targetId
    );

    // Reconstruct path as list of edge IDs
    static std::vector<int> reconstructPath(
        const Grid& grid,
        const AStarResult& result,
        int targetId
    );

    // Convenience: returns (distance, edge_path)
    static std::pair<double, std::vector<int>> shortestPath(
        const Grid& grid,
        int sourceId,
        int targetId
    );

private:
    static double heuristic(const Node& a, const Node& b);
};

} // namespace gridpulse