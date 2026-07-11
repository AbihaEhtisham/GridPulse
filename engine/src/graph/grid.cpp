#include "graph/grid.hpp"
#include <stdexcept>
#include <algorithm>

namespace gridpulse {

Grid::Grid() : nextNodeId_(0), nextEdgeId_(0) {}

// ==================== Node Operations ====================

int Grid::addNode(NodeType type, double capacityMW, CriticalityTier tier, double x, double y) {
    int id = nextNodeId_++;
    nodes_.emplace_back(id, type, capacityMW, tier, x, y);
    adjacency_.emplace_back();     // empty neighbor list for new node
    incidentEdges_.emplace_back(); // empty edge list for new node
    return id;
}

const Node& Grid::getNode(int id) const {
    if (id < 0 || id >= static_cast<int>(nodes_.size())) {
        throw std::out_of_range("Node ID out of range");
    }
    return nodes_[id];
}

Node& Grid::getNode(int id) {
    if (id < 0 || id >= static_cast<int>(nodes_.size())) {
        throw std::out_of_range("Node ID out of range");
    }
    return nodes_[id];
}

const std::vector<Node>& Grid::getNodes() const {
    return nodes_;
}

int Grid::nodeCount() const {
    return static_cast<int>(nodes_.size());
}

// ==================== Edge Operations ====================

int Grid::addEdge(int from, int to, double capacityMW, double resistance) {
    if (from < 0 || from >= static_cast<int>(nodes_.size()) ||
        to < 0 || to >= static_cast<int>(nodes_.size())) {
        throw std::out_of_range("Node ID out of range for edge creation");
    }
    if (from == to) {
        throw std::invalid_argument("Self-loops are not allowed");
    }

    int id = nextEdgeId_++;
    edges_.emplace_back(id, from, to, capacityMW, resistance);

    // Update adjacency lists (undirected graph)
    adjacency_[from].push_back(to);
    adjacency_[to].push_back(from);

    // Update incident edge lists
    incidentEdges_[from].push_back(id);
    incidentEdges_[to].push_back(id);

    return id;
}

const Edge& Grid::getEdge(int id) const {
    if (id < 0 || id >= static_cast<int>(edges_.size())) {
        throw std::out_of_range("Edge ID out of range");
    }
    return edges_[id];
}

Edge& Grid::getEdge(int id) {
    if (id < 0 || id >= static_cast<int>(edges_.size())) {
        throw std::out_of_range("Edge ID out of range");
    }
    return edges_[id];
}

const std::vector<Edge>& Grid::getEdges() const {
    return edges_;
}

int Grid::edgeCount() const {
    return static_cast<int>(edges_.size());
}

// ==================== Neighbor Queries ====================

const std::vector<int>& Grid::getNeighbors(int nodeId) const {
    if (nodeId < 0 || nodeId >= static_cast<int>(adjacency_.size())) {
        throw std::out_of_range("Node ID out of range");
    }
    return adjacency_[nodeId];
}

const std::vector<int>& Grid::getIncidentEdges(int nodeId) const {
    if (nodeId < 0 || nodeId >= static_cast<int>(incidentEdges_.size())) {
        throw std::out_of_range("Node ID out of range");
    }
    return incidentEdges_[nodeId];
}

// ==================== Bulk Operations ====================

void Grid::removeNode(int id) {
    // Soft delete: mark node as inactive
    // Full removal with adjacency cleanup is complex;
    // For simulation, we zero out capacity and disconnect edges
    if (id < 0 || id >= static_cast<int>(nodes_.size())) return;

    nodes_[id].capacityMW = 0.0;
    nodes_[id].currentLoadMW = 0.0;

    // Trip all incident edges
    for (int edgeId : incidentEdges_[id]) {
        edges_[edgeId].tripped = true;
    }
}

void Grid::removeEdge(int id) {
    if (id < 0 || id >= static_cast<int>(edges_.size())) return;
    edges_[id].tripped = true;
}

void Grid::resetFlows() {
    for (auto& node : nodes_) {
        node.currentLoadMW = 0.0;
    }
    for (auto& edge : edges_) {
        edge.currentFlowMW = 0.0;
    }
}

// ==================== Connectivity Check ====================

void Grid::dfs(int nodeId, std::vector<bool>& visited) const {
    visited[nodeId] = true;
    for (int neighbor : adjacency_[nodeId]) {
        if (!visited[neighbor]) {
            // Only traverse active edges
            for (int edgeId : incidentEdges_[nodeId]) {
                const Edge& edge = edges_[edgeId];
                if ((edge.from == nodeId && edge.to == neighbor) ||
                    (edge.to == nodeId && edge.from == neighbor)) {
                    if (!edge.tripped) {
                        dfs(neighbor, visited);
                    }
                    break;
                }
            }
        }
    }
}

bool Grid::isConnected() const {
    if (nodes_.empty()) return true;
    std::vector<bool> visited(nodes_.size(), false);
    dfs(0, visited);
    for (bool v : visited) {
        if (!v) return false;
    }
    return true;
}

} // namespace gridpulse