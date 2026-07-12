#include "recovery/recovery_manager.hpp"
#include <cmath>

namespace gridpulse {

RecoveryManager::RecoveryManager(const RecoveryConfig& config)
    : config_(config) {}

void RecoveryManager::initialize(const Grid& grid) {
    grid_ = grid;
    damageSites_.clear();
    crews_.clear();

    findDamageSites();

    // Create crews starting from depot
    for (int i = 0; i < config_.numCrews; i++) {
        RepairCrew crew;
        crew.id = i;
        crew.depotNodeId = config_.depotNodeId;
        const Node& depot = grid_.getNode(config_.depotNodeId);
        crew.x = depot.x;
        crew.y = depot.y;
        crew.currentTarget = -1;
        crew.progress = 0.0;
        crews_.push_back(crew);
    }
}

void RecoveryManager::findDamageSites() {
    for (int i = 0; i < grid_.edgeCount(); i++) {
        const Edge& edge = grid_.getEdge(i);
        if (edge.tripped) {
            DamageSite site;
            site.edgeId = i;
            site.nodeId1 = edge.from;
            site.nodeId2 = edge.to;
            const Node& n1 = grid_.getNode(edge.from);
            const Node& n2 = grid_.getNode(edge.to);
            site.x = (n1.x + n2.x) / 2.0;
            site.y = (n1.y + n2.y) / 2.0;
            site.repaired = false;
            damageSites_.push_back(site);
        }
    }
}

std::vector<Assignment> RecoveryManager::autoAssign() {
    int numCrews = static_cast<int>(crews_.size());
    int numSites = static_cast<int>(damageSites_.size());

    if (numCrews == 0 || numSites == 0) return {};

    // Build cost matrix: cost = distance from crew depot to damage site
    std::vector<std::vector<double>> costMatrix(numCrews, std::vector<double>(numSites, 0.0));

    for (int i = 0; i < numCrews; i++) {
        for (int j = 0; j < numSites; j++) {
            double dx = crews_[i].x - damageSites_[j].x;
            double dy = crews_[i].y - damageSites_[j].y;
            costMatrix[i][j] = std::sqrt(dx * dx + dy * dy);
        }
    }

    return Hungarian::solve(costMatrix);
}

std::pair<double, std::vector<int>> RecoveryManager::routeToSite(int crewId, int siteIndex) {
    if (crewId < 0 || crewId >= static_cast<int>(crews_.size()) ||
        siteIndex < 0 || siteIndex >= static_cast<int>(damageSites_.size())) {
        return {-1.0, {}};
    }

    // Find nearest node to crew position
    int sourceNode = crews_[crewId].depotNodeId;
    double bestDist = 1e9;
    for (int i = 0; i < grid_.nodeCount(); i++) {
        const Node& n = grid_.getNode(i);
        double dx = crews_[crewId].x - n.x;
        double dy = crews_[crewId].y - n.y;
        double d = std::sqrt(dx * dx + dy * dy);
        if (d < bestDist) {
            bestDist = d;
            sourceNode = i;
        }
    }

    // Find nearest node to damage site
    int targetNode = damageSites_[siteIndex].nodeId1;
    bestDist = 1e9;
    for (int i = 0; i < grid_.nodeCount(); i++) {
        const Node& n = grid_.getNode(i);
        double dx = damageSites_[siteIndex].x - n.x;
        double dy = damageSites_[siteIndex].y - n.y;
        double d = std::sqrt(dx * dx + dy * dy);
        if (d < bestDist) {
            bestDist = d;
            targetNode = i;
        }
    }

    return AStar::shortestPath(grid_, sourceNode, targetNode);
}

} // namespace gridpulse