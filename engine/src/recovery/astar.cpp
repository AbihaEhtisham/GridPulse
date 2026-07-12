#include "recovery/astar.hpp"
#include <queue>
#include <cmath>
#include <algorithm>

namespace gridpulse {

double AStar::heuristic(const Node& a, const Node& b) {
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

AStarResult AStar::findPath(const Grid& grid, int sourceId, int targetId) {
    AStarResult result;
    int n = grid.nodeCount();

    if (n == 0 || sourceId < 0 || sourceId >= n || targetId < 0 || targetId >= n) {
        return result;
    }

    result.distance.assign(n, std::numeric_limits<double>::infinity());
    result.predecessor.assign(n, -1);
    result.predecessorEdge.assign(n, -1);
    result.distance[sourceId] = 0.0;

    // Priority queue: (f_score, nodeId)
    using PQEntry = std::pair<double, int>;
    std::priority_queue<PQEntry, std::vector<PQEntry>, std::greater<PQEntry>> pq;
    pq.push({heuristic(grid.getNode(sourceId), grid.getNode(targetId)), sourceId});

    while (!pq.empty()) {
        auto [fScore, u] = pq.top();
        pq.pop();

        if (u == targetId) {
            result.pathFound = true;
            return result;
        }

        double gScore = result.distance[u];

        const auto& incidentEdges = grid.getIncidentEdges(u);
        for (int edgeId : incidentEdges) {
            const Edge& edge = grid.getEdge(edgeId);
            if (edge.tripped) continue;

            int v = (edge.from == u) ? edge.to : edge.from;
            double newG = gScore + edge.resistance;

            if (newG < result.distance[v]) {
                result.distance[v] = newG;
                result.predecessor[v] = u;
                result.predecessorEdge[v] = edgeId;
                double h = heuristic(grid.getNode(v), grid.getNode(targetId));
                pq.push({newG + h, v});
            }
        }
    }

    return result;
}

std::vector<int> AStar::reconstructPath(
    const Grid& grid,
    const AStarResult& result,
    int targetId
) {
    std::vector<int> path;
    if (!result.pathFound) return path;
    if (targetId < 0 || targetId >= static_cast<int>(result.predecessor.size())) return path;

    int current = targetId;
    while (result.predecessorEdge[current] != -1) {
        path.push_back(result.predecessorEdge[current]);
        current = result.predecessor[current];
    }
    std::reverse(path.begin(), path.end());
    return path;
}

std::pair<double, std::vector<int>> AStar::shortestPath(
    const Grid& grid,
    int sourceId,
    int targetId
) {
    AStarResult result = findPath(grid, sourceId, targetId);
    if (!result.pathFound) {
        return {-1.0, {}};
    }
    double distance = result.distance[targetId];
    std::vector<int> path = reconstructPath(grid, result, targetId);
    return {distance, path};
}

} // namespace gridpulse