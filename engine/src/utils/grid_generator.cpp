#include "utils/grid_generator.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <set>

namespace gridpulse {

// ==================== Constructor ====================

GridGenerator::GridGenerator(const GeneratorConfig& config)
    : config_(config)
    , rng_(config.seed) {}

// ==================== Public Interface ====================

Grid GridGenerator::generate() {
    Grid grid;

    // Step 1: Create all nodes with positions
    placeNodes(grid);

    // Step 2: Assign types (generator, substation, load)
    assignNodeTypes(grid);

    // Step 3: Assign criticality tiers to loads
    assignCriticalityTiers(grid);

    // Step 4: Collect all node IDs
    std::vector<int> nodeIds;
    for (const auto& node : grid.getNodes()) {
        nodeIds.push_back(node.id);
    }

    // Step 5: Build spanning tree (guarantees connectivity)
    buildSpanningTree(grid, nodeIds);

    // Step 6: Add redundant edges for resilience
    addRedundantEdges(grid, nodeIds);

    return grid;
}

Grid GridGenerator::generate(const GeneratorConfig& config) {
    GridGenerator generator(config);
    return generator.generate();
}

// ==================== Private: Node Placement ====================

void GridGenerator::placeNodes(Grid& grid) {
    double margin = 50.0;
    double w = config_.canvasWidth - 2 * margin;
    double h = config_.canvasHeight - 2 * margin;

    for (int i = 0; i < config_.totalNodes; ++i) {
        double x = margin + randomDouble(0.0, w);
        double y = margin + randomDouble(0.0, h);
        // Placeholder type and tier — will be overwritten
        grid.addNode(NodeType::LOAD, 0.0, CriticalityTier::RESIDENTIAL, x, y);
    }
}

// ==================== Private: Assign Node Types ====================

void GridGenerator::assignNodeTypes(Grid& grid) {
    int totalNodes = grid.nodeCount();
    int numGenerators = std::max(1, static_cast<int>(totalNodes * config_.generatorRatio));
    int numSubstations = std::max(1, static_cast<int>(totalNodes * config_.substationRatio));

    // Make sure we don't exceed total
    if (numGenerators + numSubstations >= totalNodes) {
        numGenerators = std::max(1, totalNodes / 3);
        numSubstations = std::max(1, totalNodes / 3);
    }

    // Create shuffled index list
    std::vector<int> indices(totalNodes);
    for (int i = 0; i < totalNodes; ++i) indices[i] = i;
    shuffle(indices);

    // Assign generators
    for (int i = 0; i < numGenerators; ++i) {
        Node& node = grid.getNode(indices[i]);
        node.type = NodeType::GENERATOR;
        node.capacityMW = randomDouble(80.0, 200.0);
    }

    // Assign substations
    for (int i = numGenerators; i < numGenerators + numSubstations; ++i) {
        Node& node = grid.getNode(indices[i]);
        node.type = NodeType::SUBSTATION;
        node.capacityMW = randomDouble(100.0, 300.0);
    }

    // Remaining nodes stay as LOAD (already set in placeNodes)
    for (int i = numGenerators + numSubstations; i < totalNodes; ++i) {
        Node& node = grid.getNode(indices[i]);
        node.capacityMW = randomDouble(10.0, 60.0); // demand capacity
    }
}

// ==================== Private: Assign Criticality Tiers ====================

void GridGenerator::assignCriticalityTiers(Grid& grid) {
    // Collect load node IDs
    std::vector<int> loadIds;
    for (const auto& node : grid.getNodes()) {
        if (node.type == NodeType::LOAD) {
            loadIds.push_back(node.id);
        }
    }
    if (loadIds.empty()) return;

    shuffle(loadIds);

    int total = static_cast<int>(loadIds.size());
    int numCritical = std::max(1, static_cast<int>(total * 0.05));
    int numCommercial = std::max(1, static_cast<int>(total * 0.20));

    for (int i = 0; i < total; ++i) {
        Node& node = grid.getNode(loadIds[i]);
        if (i < numCritical) {
            node.tier = CriticalityTier::CRITICAL;
        } else if (i < numCritical + numCommercial) {
            node.tier = CriticalityTier::COMMERCIAL;
        } else {
            node.tier = CriticalityTier::RESIDENTIAL;
        }
    }
}

// ==================== Private: Spanning Tree (Prim's Algorithm) ====================

void GridGenerator::buildSpanningTree(Grid& grid, const std::vector<int>& nodeIds) {
    int n = static_cast<int>(nodeIds.size());
    if (n <= 1) return;

    std::vector<bool> inTree(n, false);
    inTree[0] = true;
    int edgesAdded = 0;

    // Prim's algorithm: repeatedly add the cheapest edge connecting
    // a tree node to a non-tree node
    while (edgesAdded < n - 1) {
        double bestDist = 1e18;
        int bestFrom = -1;
        int bestTo = -1;

        for (int i = 0; i < n; ++i) {
            if (!inTree[i]) continue;
            for (int j = 0; j < n; ++j) {
                if (inTree[j]) continue;
                const Node& ni = grid.getNode(nodeIds[i]);
                const Node& nj = grid.getNode(nodeIds[j]);
                double dx = ni.x - nj.x;
                double dy = ni.y - nj.y;
                double dist = std::sqrt(dx * dx + dy * dy);
                if (dist < bestDist) {
                    bestDist = dist;
                    bestFrom = nodeIds[i];
                    bestTo = nodeIds[j];
                }
            }
        }

        if (bestFrom >= 0 && bestTo >= 0) {
            double capacity = randomDouble(30.0, 100.0);
            double resistance = bestDist / 100.0; // scale distance to reasonable resistance
            grid.addEdge(bestFrom, bestTo, capacity, resistance);

            // Mark the newly added node as in-tree
            for (int k = 0; k < n; ++k) {
                if (nodeIds[k] == bestTo) {
                    inTree[k] = true;
                    break;
                }
            }
            ++edgesAdded;
        }
    }
}

// ==================== Private: Redundant Edges ====================

void GridGenerator::addRedundantEdges(Grid& grid, const std::vector<int>& nodeIds) {
    int n = static_cast<int>(nodeIds.size());

    // Collect existing edges as pairs (unordered)
    std::set<std::pair<int, int>> existingPairs;
    for (const auto& edge : grid.getEdges()) {
        int a = std::min(edge.from, edge.to);
        int b = std::max(edge.from, edge.to);
        existingPairs.insert({a, b});
    }

    int added = 0;
    int maxAttempts = config_.redundancyFactor * 20; // avoid infinite loop
    int attempts = 0;

    while (added < config_.redundancyFactor && attempts < maxAttempts) {
        ++attempts;
        int i = randomInt(0, n - 1);
        int j = randomInt(0, n - 1);
        if (i == j) continue;

        int a = std::min(nodeIds[i], nodeIds[j]);
        int b = std::max(nodeIds[i], nodeIds[j]);

        // Don't duplicate existing edges
        if (existingPairs.count({a, b}) > 0) continue;

        const Node& ni = grid.getNode(a);
        const Node& nj = grid.getNode(b);
        double dx = ni.x - nj.x;
        double dy = ni.y - nj.y;
        double dist = std::sqrt(dx * dx + dy * dy);

        double capacity = randomDouble(20.0, 80.0);
        double resistance = dist / 100.0;

        grid.addEdge(a, b, capacity, resistance);
        existingPairs.insert({a, b});
        ++added;
    }
}

// ==================== Private: Random Utilities ====================

double GridGenerator::randomDouble(double min, double max) {
    std::uniform_real_distribution<double> dist(min, max);
    return dist(rng_);
}

int GridGenerator::randomInt(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng_);
}

void GridGenerator::shuffle(std::vector<int>& vec) {
    std::shuffle(vec.begin(), vec.end(), rng_);
}

// ==================== JSON Serialization (Minimal) ====================

std::string gridToJson(const Grid& grid) {
    std::ostringstream oss;

    oss << "{\n";
    oss << "  \"nodes\": [\n";
    for (size_t i = 0; i < grid.getNodes().size(); ++i) {
        const auto& n = grid.getNodes()[i];
        oss << "    {";
        oss << "\"id\": " << n.id << ", ";
        oss << "\"type\": \"" << n.typeString() << "\", ";
        oss << "\"capacityMW\": " << n.capacityMW << ", ";
        oss << "\"currentLoadMW\": " << n.currentLoadMW << ", ";
        oss << "\"tier\": \"" << n.tierString() << "\", ";
        oss << "\"x\": " << n.x << ", ";
        oss << "\"y\": " << n.y;
        oss << "}";
        if (i < grid.getNodes().size() - 1) oss << ",";
        oss << "\n";
    }
    oss << "  ],\n";

    oss << "  \"edges\": [\n";
    for (size_t i = 0; i < grid.getEdges().size(); ++i) {
        const auto& e = grid.getEdges()[i];
        oss << "    {";
        oss << "\"id\": " << e.id << ", ";
        oss << "\"from\": " << e.from << ", ";
        oss << "\"to\": " << e.to << ", ";
        oss << "\"capacityMW\": " << e.capacityMW << ", ";
        oss << "\"currentFlowMW\": " << e.currentFlowMW << ", ";
        oss << "\"resistance\": " << e.resistance << ", ";
        oss << "\"tripped\": " << (e.tripped ? "true" : "false");
        oss << "}";
        if (i < grid.getEdges().size() - 1) oss << ",";
        oss << "\n";
    }
    oss << "  ]\n";
    oss << "}\n";

    return oss.str();
}

// Minimal JSON parser — for production, use nlohmann/json or similar
Grid gridFromJson(const std::string& json) {
    // Placeholder: in practice, use a proper JSON library
    // For now, we use the generator and serialize; deserialization comes later
    Grid grid;
    // TODO: Implement proper JSON parsing
    return grid;
}

} // namespace gridpulse