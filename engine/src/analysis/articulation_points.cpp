#include "analysis/articulation_points.hpp"
#include <algorithm>
#include <sstream>

namespace gridpulse {

ArticulationResult ArticulationPoints::find(const Grid& grid) {
    ArticulationResult result;
    int n = grid.nodeCount();
    result.totalNodes = n;
    
    if (n == 0) return result;
    
    std::vector<bool> visited(n, false);
    std::vector<int> discoveryTime(n, -1);
    std::vector<int> lowTime(n, -1);
    std::vector<bool> isArticulation(n, false);
    int time = 0;
    
    // Handle disconnected graphs: run DFS from each unvisited node
    for (int i = 0; i < n; ++i) {
        if (!visited[i]) {
            dfs(grid, i, -1, visited, discoveryTime, lowTime, isArticulation, time);
        }
    }
    
    // Collect results
    for (int i = 0; i < n; ++i) {
        if (isArticulation[i]) {
            result.articulationPoints.push_back(i);
        }
    }
    
    result.articulationCount = static_cast<int>(result.articulationPoints.size());
    return result;
}

bool ArticulationPoints::isArticulationPoint(const Grid& grid, int nodeId) {
    ArticulationResult result = find(grid);
    return std::find(result.articulationPoints.begin(),
                     result.articulationPoints.end(),
                     nodeId) != result.articulationPoints.end();
}

void ArticulationPoints::dfs(
    const Grid& grid,
    int u,
    int parent,
    std::vector<bool>& visited,
    std::vector<int>& discoveryTime,
    std::vector<int>& lowTime,
    std::vector<bool>& isArticulation,
    int& time
) {
    visited[u] = true;
    discoveryTime[u] = lowTime[u] = ++time;
    int children = 0;
    
    for (int edgeId : grid.getIncidentEdges(u)) {
        const Edge& edge = grid.getEdge(edgeId);
        if (edge.tripped) continue;
        
        int v = (edge.from == u) ? edge.to : edge.from;
        
        if (!visited[v]) {
            children++;
            dfs(grid, v, u, visited, discoveryTime, lowTime, isArticulation, time);
            
            lowTime[u] = std::min(lowTime[u], lowTime[v]);
            
            // Articulation point condition 1: u is root and has >= 2 children
            if (parent == -1 && children > 1) {
                isArticulation[u] = true;
            }
            
            // Articulation point condition 2: u is not root and low[v] >= disc[u]
            if (parent != -1 && lowTime[v] >= discoveryTime[u]) {
                isArticulation[u] = true;
            }
        } else if (v != parent) {
            lowTime[u] = std::min(lowTime[u], discoveryTime[v]);
        }
    }
}

std::string ArticulationPoints::summary(const ArticulationResult& result, const Grid& grid) {
    std::ostringstream oss;
    oss << "Articulation Point Analysis\n";
    oss << "===========================\n";
    oss << "Total nodes: " << result.totalNodes << "\n";
    oss << "Articulation points found: " << result.articulationCount << "\n\n";
    
    if (result.articulationPoints.empty()) {
        oss << "No articulation points. The grid is 2-vertex-connected.\n";
    } else {
        oss << "Articulation points (single points of failure):\n";
        for (int id : result.articulationPoints) {
            const Node& node = grid.getNode(id);
            oss << "  Node " << id << " | Type: " << node.typeString()
                << " | Tier: " << node.tierString() << "\n";
        }
    }
    
    return oss.str();
}

} // namespace gridpulse