#include "analysis/betweenness.hpp"
#include <queue>
#include <stack>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <limits>

namespace gridpulse {

BetweennessResult Betweenness::compute(const Grid& grid) {
    BetweennessResult result;
    int n = grid.nodeCount();
    int m = grid.edgeCount();
    
    if (n == 0) return result;
    
    result.totalNodesAnalyzed = n;
    result.totalEdgesAnalyzed = m;
    
    // Initialize accumulators
    std::vector<double> nodeBetweenness(n, 0.0);
    std::vector<double> edgeBetweenness(m, 0.0);
    
    // Brandes' algorithm: run shortest paths from each source node
    for (int s = 0; s < n; ++s) {
        // Skip isolated nodes (no active edges)
        bool hasActiveEdge = false;
        for (int edgeId : grid.getIncidentEdges(s)) {
            if (!grid.getEdge(edgeId).tripped) {
                hasActiveEdge = true;
                break;
            }
        }
        if (!hasActiveEdge) continue;
        
        // Data structures for this source
        std::stack<int> stack;
        std::vector<std::vector<int>> predecessors(n);
        std::vector<double> sigma(n, 0.0);      // number of shortest paths
        std::vector<double> distance(n, std::numeric_limits<double>::infinity());
        std::vector<double> delta(n, 0.0);
        
        sigma[s] = 1.0;
        distance[s] = 0.0;
        
        std::queue<int> q;
        q.push(s);
        
        // BFS to find shortest paths (using resistance as weight is approximate;
        // for strict Brandes, we use BFS on unweighted. For weighted, we use Dijkstra.)
        // Here we use BFS treating all active edges as unit weight for speed.
        // For production, swap with Dijkstra for resistance-weighted betweenness.
        while (!q.empty()) {
            int v = q.front();
            q.pop();
            stack.push(v);
            
            for (int edgeId : grid.getIncidentEdges(v)) {
                const Edge& edge = grid.getEdge(edgeId);
                if (edge.tripped) continue;
                
                int w = (edge.from == v) ? edge.to : edge.from;
                
                // Path discovery
                if (distance[w] == std::numeric_limits<double>::infinity()) {
                    distance[w] = distance[v] + 1.0;
                    q.push(w);
                }
                
                // Path counting
                if (distance[w] == distance[v] + 1.0) {
                    sigma[w] += sigma[v];
                    predecessors[w].push_back(v);
                }
            }
        }
        
        // Back-propagation of dependencies
        while (!stack.empty()) {
            int w = stack.top();
            stack.pop();
            
            for (int v : predecessors[w]) {
                double contribution = (sigma[v] / sigma[w]) * (1.0 + delta[w]);
                delta[v] += contribution;
                
                // Find the edge(s) between v and w and accumulate
                for (int edgeId : grid.getIncidentEdges(v)) {
                    const Edge& edge = grid.getEdge(edgeId);
                    if (edge.tripped) continue;
                    int neighbor = (edge.from == v) ? edge.to : edge.from;
                    if (neighbor == w) {
                        edgeBetweenness[edgeId] += contribution;
                        break;
                    }
                }
            }
            
            if (w != s) {
                nodeBetweenness[w] += delta[w];
            }
        }
    }
    
    // Normalize edge betweenness (undirected graph: divide by 2)
    for (auto& score : edgeBetweenness) {
        score /= 2.0;
    }
    
    // Build sorted results
    for (int i = 0; i < n; ++i) {
        result.nodeScores.push_back({i, nodeBetweenness[i]});
        result.maxNodeScore = std::max(result.maxNodeScore, nodeBetweenness[i]);
    }
    
    for (int i = 0; i < m; ++i) {
        result.edgeScores.push_back({i, edgeBetweenness[i]});
        result.maxEdgeScore = std::max(result.maxEdgeScore, edgeBetweenness[i]);
    }
    
    // Sort descending by score
    std::sort(result.nodeScores.begin(), result.nodeScores.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    std::sort(result.edgeScores.begin(), result.edgeScores.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    return result;
}

std::vector<std::pair<int, double>> Betweenness::topEdges(
    const BetweennessResult& result, int k
) {
    std::vector<std::pair<int, double>> top;
    int count = std::min(k, static_cast<int>(result.edgeScores.size()));
    for (int i = 0; i < count; ++i) {
        top.push_back(result.edgeScores[i]);
    }
    return top;
}

std::vector<std::pair<int, double>> Betweenness::topNodes(
    const BetweennessResult& result, int k
) {
    std::vector<std::pair<int, double>> top;
    int count = std::min(k, static_cast<int>(result.nodeScores.size()));
    for (int i = 0; i < count; ++i) {
        top.push_back(result.nodeScores[i]);
    }
    return top;
}

std::string Betweenness::summary(const BetweennessResult& result) {
    std::ostringstream oss;
    oss << "Betweenness Centrality Summary\n";
    oss << "==============================\n";
    oss << "Nodes analyzed: " << result.totalNodesAnalyzed << "\n";
    oss << "Edges analyzed: " << result.totalEdgesAnalyzed << "\n";
    oss << "Max node score: " << std::fixed << std::setprecision(4) << result.maxNodeScore << "\n";
    oss << "Max edge score: " << std::fixed << std::setprecision(4) << result.maxEdgeScore << "\n\n";
    
    oss << "Top 5 Critical Edges:\n";
    auto topE = Betweenness::topEdges(result, 5);
    for (const auto& [id, score] : topE) {
        oss << "  Edge " << id << ": " << std::fixed << std::setprecision(4) << score << "\n";
    }
    
    oss << "\nTop 5 Critical Nodes:\n";
    auto topN = Betweenness::topNodes(result, 5);
    for (const auto& [id, score] : topN) {
        oss << "  Node " << id << ": " << std::fixed << std::setprecision(4) << score << "\n";
    }
    
    return oss.str();
}

} // namespace gridpulse