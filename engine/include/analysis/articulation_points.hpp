#pragma once

#include <vector>
#include <string>
#include "graph/grid.hpp"

namespace gridpulse {

struct ArticulationResult {
    std::vector<int> articulationPoints;  // node IDs that are articulation points
    int totalNodes;
    int articulationCount;
    
    ArticulationResult() : totalNodes(0), articulationCount(0) {}
};

class ArticulationPoints {
public:
    // Find all articulation points using Tarjan's algorithm
    // O(V + E) time complexity
    static ArticulationResult find(const Grid& grid);
    
    // Check if a specific node is an articulation point
    static bool isArticulationPoint(const Grid& grid, int nodeId);
    
    // Convert result to summary string
    static std::string summary(const ArticulationResult& result, const Grid& grid);
    
private:
    static void dfs(
        const Grid& grid,
        int u,
        int parent,
        std::vector<bool>& visited,
        std::vector<int>& discoveryTime,
        std::vector<int>& lowTime,
        std::vector<bool>& isArticulation,
        int& time
    );
};

} // namespace gridpulse