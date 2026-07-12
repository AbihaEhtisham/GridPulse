#pragma once

#include <vector>
#include <string>
#include "graph/grid.hpp"
#include "recovery/astar.hpp"
#include "recovery/hungarian.hpp"

namespace gridpulse {

struct DamageSite {
    int edgeId;
    int nodeId1;
    int nodeId2;
    double x, y;  // midpoint for crew routing
    bool repaired;
};

struct RepairCrew {
    int id;
    int depotNodeId;
    double x, y;        // current position
    int currentTarget;  // damage site index, -1 if idle
    double progress;    // 0.0 to 1.0
};

struct RecoveryConfig {
    int numCrews;
    int depotNodeId;   // where crews start from
};

class RecoveryManager {
public:
    RecoveryManager(const RecoveryConfig& config);

    // Initialize with damaged grid
    void initialize(const Grid& grid);

    // Auto-assign crews to damage sites using Hungarian algorithm
    std::vector<Assignment> autoAssign();

    // Get the optimal path for a crew to a damage site using A*
    std::pair<double, std::vector<int>> routeToSite(int crewId, int siteIndex);

    // Get all damage sites
    const std::vector<DamageSite>& getDamageSites() const { return damageSites_; }

    // Get all crews
    const std::vector<RepairCrew>& getCrews() const { return crews_; }

    // Get the grid reference
    const Grid& getGrid() const { return grid_; }

private:
    RecoveryConfig config_;
    Grid grid_;
    std::vector<DamageSite> damageSites_;
    std::vector<RepairCrew> crews_;

    void findDamageSites();
};

} // namespace gridpulse