#pragma once

#include <vector>
#include <string>
#include "node.hpp"
#include "edge.hpp"

namespace gridpulse {

class Grid {
public:
    Grid();

    // Node operations
    int addNode(NodeType type, double capacityMW, CriticalityTier tier, double x, double y);
    const Node& getNode(int id) const;
    Node& getNode(int id);
    const std::vector<Node>& getNodes() const;
    int nodeCount() const;

    // Edge operations
    int addEdge(int from, int to, double capacityMW, double resistance);
    const Edge& getEdge(int id) const;
    Edge& getEdge(int id);
    const std::vector<Edge>& getEdges() const;
    int edgeCount() const;

    // Neighbor queries (from adjacency list)
    const std::vector<int>& getNeighbors(int nodeId) const;
    const std::vector<int>& getIncidentEdges(int nodeId) const;

    // Bulk operations
    void removeNode(int id);
    void removeEdge(int id);
    void resetFlows();

    // Validation
    bool isConnected() const;  // checks if all nodes are in one component

private:
    std::vector<Node> nodes_;
    std::vector<Edge> edges_;
    std::vector<std::vector<int>> adjacency_;      // node ID -> list of neighbor node IDs
    std::vector<std::vector<int>> incidentEdges_;  // node ID -> list of edge IDs
    int nextNodeId_;
    int nextEdgeId_;

    void dfs(int nodeId, std::vector<bool>& visited) const;
};

} // namespace gridpulse