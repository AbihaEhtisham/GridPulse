#pragma once

#include <vector>
#include <utility>
#include <string>
#include "graph/grid.hpp"

namespace gridpulse {

struct BetweennessResult {
    // Pair of (edgeId, betweennessScore), sorted descending by score
    std::vector<std::pair<int, double>> edgeScores;
    
    // Pair of (nodeId, betweennessScore), sorted descending by score
    std::vector<std::pair<int, double>> nodeScores;
    
    double maxEdgeScore;
    double maxNodeScore;
    int totalEdgesAnalyzed;
    int totalNodesAnalyzed;
    
    BetweennessResult() : maxEdgeScore(0.0), maxNodeScore(0.0),
                          totalEdgesAnalyzed(0), totalNodesAnalyzed(0) {}
};

class Betweenness {
public:
    // Compute betweenness centrality for all nodes and edges
    // Uses Brandes' algorithm: O(V * E) for unweighted graphs
    // Edge weights (resistance) are used as path weights
    static BetweennessResult compute(const Grid& grid);
    
    // Get only the top-K most critical edges
    static std::vector<std::pair<int, double>> topEdges(
        const BetweennessResult& result, int k
    );
    
    // Get only the top-K most critical nodes
    static std::vector<std::pair<int, double>> topNodes(
        const BetweennessResult& result, int k
    );
    
    // Convert result to a summary string
    static std::string summary(const BetweennessResult& result);
};

} // namespace gridpulse