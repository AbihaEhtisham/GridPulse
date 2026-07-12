#include "utils/map_generator.hpp"
#include <algorithm>
#include <cmath>
#include <set>

namespace gridpulse {

MapGridGenerator::MapGridGenerator(const MapConfig& config)
    : config_(config), rng_(config.seed) {}

Grid MapGridGenerator::generate() {
    Grid grid;

    // Place nodes in order: plants → substations → critical → commercial → residential
    placePowerPlants(grid);
    std::vector<int> plantIds;
    for (const auto& n : grid.getNodes()) {
        if (n.type == NodeType::GENERATOR) plantIds.push_back(n.id);
    }

    placeSubstations(grid);
    std::vector<int> subIds;
    for (const auto& n : grid.getNodes()) {
        if (n.type == NodeType::SUBSTATION) subIds.push_back(n.id);
    }

    placeCriticalLoads(grid);
    std::vector<int> critIds;
    for (const auto& n : grid.getNodes()) {
        if (n.type == NodeType::LOAD && n.tier == CriticalityTier::CRITICAL)
            critIds.push_back(n.id);
    }

    placeCommercialLoads(grid);
    std::vector<int> commIds;
    for (const auto& n : grid.getNodes()) {
        if (n.type == NodeType::LOAD && n.tier == CriticalityTier::COMMERCIAL)
            commIds.push_back(n.id);
    }

    placeResidentialLoads(grid);
    std::vector<int> resIds;
    for (const auto& n : grid.getNodes()) {
        if (n.type == NodeType::LOAD && n.tier == CriticalityTier::RESIDENTIAL)
            resIds.push_back(n.id);
    }

    // Build connections
    connectPowerPlants(grid, plantIds, subIds);
    connectSubstations(grid, subIds);

    // Connect all loads to nearest substation
    std::vector<int> allLoadIds;
    allLoadIds.insert(allLoadIds.end(), critIds.begin(), critIds.end());
    allLoadIds.insert(allLoadIds.end(), commIds.begin(), commIds.end());
    allLoadIds.insert(allLoadIds.end(), resIds.begin(), resIds.end());

    for (int id : allLoadIds) {
        connectToNearestSubstation(grid, id, subIds);
    }

    // Add redundant edges
    addRedundancy(grid);

    return grid;
}

Grid MapGridGenerator::generate(const MapConfig& config) {
    MapGridGenerator gen(config);
    return gen.generate();
}

// ===== Placement Functions =====

void MapGridGenerator::placePowerPlants(Grid& grid) {
    double margin = 80.0;
    double w = config_.mapWidth;
    double h = config_.mapHeight;

    for (int i = 0; i < config_.numPowerPlants; ++i) {
        double x, y;
        // Place on outskirts
        if (i == 0) {
            x = margin + randomDouble(0, w * 0.2);
            y = h - margin - randomDouble(0, h * 0.3);
        } else if (i == 1) {
            x = w - margin - randomDouble(0, w * 0.2);
            y = margin + randomDouble(0, h * 0.3);
        } else {
            x = w * 0.5 + randomDouble(-100, 100);
            y = margin + randomDouble(0, 50);
        }
        double capacity = randomDouble(150.0, 300.0);
        grid.addNode(NodeType::GENERATOR, capacity, CriticalityTier::CRITICAL, x, y);
    }
}

void MapGridGenerator::placeSubstations(Grid& grid) {
    double w = config_.mapWidth;
    double h = config_.mapHeight;
    double margin = 150.0;

    // Place in a rough grid pattern across the city
    int cols = static_cast<int>(std::ceil(std::sqrt(config_.numSubstations)));
    int rows = (config_.numSubstations + cols - 1) / cols;

    double cellW = (w - 2 * margin) / cols;
    double cellH = (h - 2 * margin) / rows;

    int placed = 0;
    for (int r = 0; r < rows && placed < config_.numSubstations; ++r) {
        for (int c = 0; c < cols && placed < config_.numSubstations; ++c) {
            double x = margin + c * cellW + cellW * 0.5 + randomDouble(-30, 30);
            double y = margin + r * cellH + cellH * 0.5 + randomDouble(-30, 30);
            double capacity = randomDouble(200.0, 400.0);
            grid.addNode(NodeType::SUBSTATION, capacity, CriticalityTier::COMMERCIAL, x, y);
            ++placed;
        }
    }
}

void MapGridGenerator::placeCriticalLoads(Grid& grid) {
    double cx = config_.mapWidth * 0.5;
    double cy = config_.mapHeight * 0.5;

    for (int i = 0; i < config_.numHospitals; ++i) {
        double x = cx + randomDouble(-120, 120);
        double y = cy + randomDouble(-80, 80);
        double demand = randomDouble(30.0, 60.0);
        grid.addNode(NodeType::LOAD, demand, CriticalityTier::CRITICAL, x, y);
    }
}

void MapGridGenerator::placeCommercialLoads(Grid& grid) {
    double cx = config_.mapWidth * 0.5;
    double cy = config_.mapHeight * 0.5;

    for (int i = 0; i < config_.numCommercial; ++i) {
        // Spread around city center
        double angle = (2.0 * 3.14159 * i) / config_.numCommercial;
        double radius = 150.0 + randomDouble(-50, 100);
        double x = cx + radius * std::cos(angle) + randomDouble(-40, 40);
        double y = cy + radius * std::sin(angle) + randomDouble(-40, 40);
        double demand = randomDouble(15.0, 40.0);
        grid.addNode(NodeType::LOAD, demand, CriticalityTier::COMMERCIAL, x, y);
    }
}

void MapGridGenerator::placeResidentialLoads(Grid& grid) {
    double w = config_.mapWidth;
    double h = config_.mapHeight;
    double margin = 60.0;

    // Create neighborhood clusters
    struct Cluster {
        double cx, cy, radius;
    };

    std::vector<Cluster> clusters = {
        {w * 0.25, h * 0.25, 80.0},
        {w * 0.75, h * 0.25, 80.0},
        {w * 0.25, h * 0.75, 80.0},
        {w * 0.75, h * 0.75, 80.0},
        {w * 0.5, h * 0.2, 60.0},
        {w * 0.5, h * 0.8, 60.0},
    };

    int perCluster = (config_.numResidential + static_cast<int>(clusters.size()) - 1) / static_cast<int>(clusters.size());
    int placed = 0;

    for (const auto& cluster : clusters) {
        for (int j = 0; j < perCluster && placed < config_.numResidential; ++j) {
            double angle = randomDouble(0, 2.0 * 3.14159);
            double r = randomDouble(0, cluster.radius);
            double x = cluster.cx + r * std::cos(angle);
            double y = cluster.cy + r * std::sin(angle);
            x = std::max(margin, std::min(w - margin, x));
            y = std::max(margin, std::min(h - margin, y));
            double demand = randomDouble(2.0, 8.0);
            grid.addNode(NodeType::LOAD, demand, CriticalityTier::RESIDENTIAL, x, y);
            ++placed;
        }
    }
}

// ===== Connection Functions =====

void MapGridGenerator::connectToNearestSubstation(Grid& grid, int nodeId, const std::vector<int>& substationIds) {
    int bestSub = -1;
    double bestDist = 1e18;

    for (int subId : substationIds) {
        double d = distance(nodeId, subId, grid);
        if (d < bestDist) {
            bestDist = d;
            bestSub = subId;
        }
    }

    if (bestSub >= 0) {
        double capacity = randomDouble(30.0, 80.0);
        double resistance = bestDist / 100.0;
        grid.addEdge(nodeId, bestSub, capacity, resistance);
    }
}

void MapGridGenerator::connectSubstations(Grid& grid, const std::vector<int>& substationIds) {
    // Connect in a ring for baseline connectivity
    for (size_t i = 0; i < substationIds.size(); ++i) {
        int from = substationIds[i];
        int to = substationIds[(i + 1) % substationIds.size()];
        double d = distance(from, to, grid);
        double capacity = randomDouble(100.0, 200.0);
        double resistance = d / 100.0;
        grid.addEdge(from, to, capacity, resistance);
    }

    // Add cross-connections for mesh topology
    if (substationIds.size() >= 4) {
        for (size_t i = 0; i < substationIds.size(); i += 2) {
            int from = substationIds[i];
            int to = substationIds[(i + 2) % substationIds.size()];
            double d = distance(from, to, grid);
            double capacity = randomDouble(80.0, 150.0);
            grid.addEdge(from, to, capacity, d / 100.0);
        }
    }
}

void MapGridGenerator::connectPowerPlants(Grid& grid, const std::vector<int>& plantIds, const std::vector<int>& substationIds) {
    for (int plantId : plantIds) {
        // Connect to 2 nearest substations for redundancy
        std::vector<std::pair<double, int>> dists;
        for (int subId : substationIds) {
            dists.push_back({distance(plantId, subId, grid), subId});
        }
        std::sort(dists.begin(), dists.end());

        int connectTo = std::min(2, static_cast<int>(dists.size()));
        for (int i = 0; i < connectTo; ++i) {
            double d = dists[i].first;
            double capacity = randomDouble(150.0, 250.0);
            grid.addEdge(plantId, dists[i].second, capacity, d / 100.0);
        }
    }
}

void MapGridGenerator::addRedundancy(Grid& grid) {
    // Collect existing pairs
    std::set<std::pair<int, int>> existing;
    for (const auto& e : grid.getEdges()) {
        existing.insert({std::min(e.from, e.to), std::max(e.from, e.to)});
    }

    // Add extra connections between substations and loads
    int added = 0;
    int maxAttempts = config_.redundancyFactor * 50;
    int attempts = 0;

    while (added < config_.redundancyFactor && attempts < maxAttempts) {
        ++attempts;
        int n1 = randomInt(0, grid.nodeCount() - 1);
        int n2 = randomInt(0, grid.nodeCount() - 1);
        if (n1 == n2) continue;

        int a = std::min(n1, n2);
        int b = std::max(n1, n2);
        if (existing.count({a, b})) continue;

        const Node& node1 = grid.getNode(a);
        const Node& node2 = grid.getNode(b);

        // Don't connect two generators or two loads directly (unrealistic)
        if (node1.type == NodeType::GENERATOR && node2.type == NodeType::GENERATOR) continue;
        if (node1.type == NodeType::LOAD && node2.type == NodeType::LOAD) continue;

        double d = distance(a, b, grid);
        double capacity = randomDouble(30.0, 100.0);
        grid.addEdge(a, b, capacity, d / 100.0);
        existing.insert({a, b});
        ++added;
    }
}

// ===== Utility =====

double MapGridGenerator::distance(int n1, int n2, const Grid& grid) const {
    const Node& a = grid.getNode(n1);
    const Node& b = grid.getNode(n2);
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

double MapGridGenerator::randomDouble(double min, double max) {
    std::uniform_real_distribution<double> dist(min, max);
    return dist(rng_);
}

int MapGridGenerator::randomInt(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng_);
}

} // namespace gridpulse