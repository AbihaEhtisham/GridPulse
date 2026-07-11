#include "flow/spfa.hpp"
#include <queue>
#include <algorithm>
#include <stdexcept>

namespace gridpulse {

SPFAResult SPFA::shortestPaths(
    const Grid& grid,
    int sourceId,
    const std::vector<double>& edgeWeights
) {
    SPFAResult result;
    int n = grid.nodeCount();

    if (n == 0 || sourceId < 0 || sourceId >= n) {
        return result;
    }

    // Validate edgeWeights size
    if (static_cast<int>(edgeWeights.size()) != grid.edgeCount()) {
        return result;
    }

    // Initialize
    result.distance.assign(n, std::numeric_limits<double>::infinity());
    result.predecessor.assign(n, -1);
    result.predecessorEdge.assign(n, -1);
    result.distance[sourceId] = 0.0;

    // Track: is node currently in queue?
    std::vector<bool> inQueue(n, false);
    // Track: how many times each node has been relaxed (for negative cycle detection)
    std::vector<int> relaxCount(n, 0);

    // Queue for BFS-style relaxation
    std::queue<int> q;
    q.push(sourceId);
    inQueue[sourceId] = true;
    relaxCount[sourceId]++;

    int iterations = 0;

    while (!q.empty() && iterations < MAX_ITERATIONS) {
        ++iterations;
        int u = q.front();
        q.pop();
        inQueue[u] = false;

        // Explore all incident edges
        const auto& incidentEdges = grid.getIncidentEdges(u);
        for (int edgeId : incidentEdges) {
            const Edge& edge = grid.getEdge(edgeId);
            if (edge.tripped) continue;

            // Determine neighbor and edge direction
            int v;
            double weight;

            if (edge.from == u) {
                v = edge.to;
                weight = edgeWeights[edgeId];
            } else {
                // Edge is undirected in our grid model
                // For residual graphs in MCMF, we handle direction there
                v = edge.from;
                weight = edgeWeights[edgeId];
            }

            double newDist = result.distance[u] + weight;
            if (newDist < result.distance[v] - 1e-9) { // tolerance for floating point
                result.distance[v] = newDist;
                result.predecessor[v] = u;
                result.predecessorEdge[v] = edgeId;

                if (!inQueue[v]) {
                    q.push(v);
                    inQueue[v] = true;
                    relaxCount[v]++;

                    // Negative cycle detection
                    if (relaxCount[v] > n) {
                        result.negativeCycle = true;
                        result.success = false;
                        return result;
                    }
                }
            }
        }
    }

    result.success = !result.negativeCycle;
    return result;
}

std::vector<int> SPFA::reconstructPath(
    const Grid& grid,
    const SPFAResult& result,
    int targetId
) {
    std::vector<int> path;
    if (!result.success) return path;
    if (targetId < 0 || targetId >= static_cast<int>(result.predecessor.size())) return path;
    if (result.distance[targetId] == std::numeric_limits<double>::infinity()) return path;

    // Detect cycles in predecessor chain (defensive)
    std::vector<bool> visited(result.predecessor.size(), false);
    int current = targetId;
    while (result.predecessorEdge[current] != -1) {
        if (visited[current]) break; // cycle detected
        visited[current] = true;
        path.push_back(result.predecessorEdge[current]);
        current = result.predecessor[current];
    }

    std::reverse(path.begin(), path.end());
    return path;
}

std::pair<double, std::vector<int>> SPFA::shortestPath(
    const Grid& grid,
    int sourceId,
    int targetId,
    const std::vector<double>& edgeWeights
) {
    SPFAResult result = shortestPaths(grid, sourceId, edgeWeights);
    if (!result.success) {
        return {-1.0, {}};
    }
    double distance = result.distance[targetId];
    std::vector<int> path = reconstructPath(grid, result, targetId);
    return {distance, path};
}

} // namespace gridpulse