#include "flow/dijkstra.hpp"
#include <queue>
#include <algorithm>
#include <stdexcept>

namespace gridpulse {

DijkstraResult Dijkstra::shortestPaths(const Grid& grid, int sourceId) {
    DijkstraResult result;
    int n = grid.nodeCount();

    if (n == 0 || sourceId < 0 || sourceId >= n) {
        return result;
    }

    // Initialize distances
    result.distance.assign(n, std::numeric_limits<double>::infinity());
    result.predecessor.assign(n, -1);
    result.predecessorEdge.assign(n, -1);
    result.distance[sourceId] = 0.0;

    // Min-heap priority queue
    std::priority_queue<HeapEntry, std::vector<HeapEntry>, std::greater<HeapEntry>> pq;
    pq.push({0.0, sourceId});

    while (!pq.empty()) {
        HeapEntry current = pq.top();
        pq.pop();

        double dist = current.distance;
        int u = current.nodeId;

        // Skip outdated entries (lazy deletion)
        if (dist > result.distance[u]) continue;

        // Explore all neighbors through active edges
        const auto& incidentEdges = grid.getIncidentEdges(u);
        for (int edgeId : incidentEdges) {
            const Edge& edge = grid.getEdge(edgeId);
            if (edge.tripped) continue;

            // Determine neighbor node
            int v = (edge.from == u) ? edge.to : edge.from;
            double weight = edge.resistance;

            double newDist = dist + weight;
            if (newDist < result.distance[v]) {
                result.distance[v] = newDist;
                result.predecessor[v] = u;
                result.predecessorEdge[v] = edgeId;
                pq.push({newDist, v});
            }
        }
    }

    result.success = true;
    return result;
}

std::vector<int> Dijkstra::reconstructPath(
    const Grid& grid,
    const DijkstraResult& result,
    int targetId
) {
    std::vector<int> path; // edge IDs
    if (!result.success) return path;
    if (targetId < 0 || targetId >= static_cast<int>(result.predecessor.size())) return path;
    if (result.distance[targetId] == std::numeric_limits<double>::infinity()) return path;

    int current = targetId;
    while (result.predecessorEdge[current] != -1) {
        path.push_back(result.predecessorEdge[current]);
        current = result.predecessor[current];
    }

    std::reverse(path.begin(), path.end());
    return path;
}

std::pair<double, std::vector<int>> Dijkstra::shortestPath(
    const Grid& grid,
    int sourceId,
    int targetId
) {
    DijkstraResult result = shortestPaths(grid, sourceId);
    if (!result.success) {
        return {-1.0, {}};
    }
    double distance = result.distance[targetId];
    std::vector<int> path = reconstructPath(grid, result, targetId);
    return {distance, path};
}

} // namespace gridpulse