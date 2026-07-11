#pragma once

#include <vector>
#include <limits>
#include "graph/grid.hpp"

namespace gridpulse {

struct DijkstraResult {
    std::vector<double> distance;     // distance from source to each node
    std::vector<int> predecessor;     // predecessor node in shortest path
    std::vector<int> predecessorEdge; // edge ID used to reach this node
    bool success;                     // true if all reachable nodes found

    DijkstraResult() : success(false) {}
};

class Dijkstra {
public:
    // Run Dijkstra from a single source node
    // Edge weight = resistance by default; can be overridden
    static DijkstraResult shortestPaths(
        const Grid& grid,
        int sourceId
    );

    // Reconstruct the path from source to a specific target
    // Returns list of edge IDs in order from source to target
    static std::vector<int> reconstructPath(
        const Grid& grid,
        const DijkstraResult& result,
        int targetId
    );

    // Compute shortest path between two specific nodes
    // Returns pair: (total_distance, list_of_edge_IDs)
    static std::pair<double, std::vector<int>> shortestPath(
        const Grid& grid,
        int sourceId,
        int targetId
    );

private:
    // Min-heap entry for priority queue
    struct HeapEntry {
        double distance;
        int nodeId;
        bool operator>(const HeapEntry& other) const {
            return distance > other.distance;
        }
    };
};

} // namespace gridpulse