#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include "graph/grid.hpp"
#include "utils/grid_generator.hpp"
#include "flow/min_cost_max_flow.hpp"

using namespace gridpulse;

int main() {
    std::cout << "=== GridPulse Engine — Min-Cost Max-Flow Test ===\n\n";

    // Test 1: Simple manually built grid
    std::cout << "=== Test 1: Simple 4-node grid ===\n\n";
    Grid grid1;

    int gen = grid1.addNode(NodeType::GENERATOR, 100.0, CriticalityTier::CRITICAL, 0.0, 0.0);
    int sub = grid1.addNode(NodeType::SUBSTATION, 200.0, CriticalityTier::COMMERCIAL, 100.0, 100.0);
    int loadCrit = grid1.addNode(NodeType::LOAD, 40.0, CriticalityTier::CRITICAL, 200.0, 0.0);
    int loadRes = grid1.addNode(NodeType::LOAD, 60.0, CriticalityTier::RESIDENTIAL, 200.0, 200.0);

    grid1.addEdge(gen, sub, 100.0, 1.0);
    grid1.addEdge(sub, loadCrit, 50.0, 2.0);
    grid1.addEdge(sub, loadRes, 70.0, 5.0);
    grid1.addEdge(gen, loadCrit, 40.0, 1.0);
    grid1.addEdge(gen, loadRes, 60.0, 3.0);

    // Run MCMF
    MCMFResult result1 = MinCostMaxFlow::compute(grid1);

    std::cout << "Grid: " << grid1.nodeCount() << " nodes, " << grid1.edgeCount() << " edges\n";
    std::cout << "Total flow delivered: " << std::fixed << std::setprecision(2)
              << result1.totalFlow << " MW\n";
    std::cout << "Total cost: " << std::fixed << std::setprecision(2) << result1.totalCost << "\n";

    std::cout << "\nEdge flows:\n";
    for (size_t i = 0; i < result1.edgeFlows.size(); ++i) {
        std::cout << "  Edge " << i << " (" << grid1.getEdge(static_cast<int>(i)).from
                  << " -> " << grid1.getEdge(static_cast<int>(i)).to
                  << "): " << std::fixed << std::setprecision(2)
                  << result1.edgeFlows[i] << " MW"
                  << " | Capacity: " << grid1.getEdge(static_cast<int>(i)).capacityMW << " MW";
        if (result1.edgeFlows[i] > grid1.getEdge(static_cast<int>(i)).capacityMW + 0.01) {
            std::cout << " [OVERLOADED!]";
        }
        std::cout << "\n";
    }

    std::cout << "\nUnmet demand nodes: ";
    if (result1.unmetDemand.empty()) {
        std::cout << "NONE (all demand satisfied)\n";
    } else {
        for (int id : result1.unmetDemand) {
            std::cout << "Node " << id << " ";
        }
        std::cout << "\n";
    }

    // Test 2: Generated grid
    std::cout << "\n\n=== Test 2: Generated 15-node grid ===\n\n";
    GeneratorConfig config;
    config.totalNodes = 15;
    config.generatorRatio = 0.2;
    config.substationRatio = 0.2;
    config.redundancyFactor = 3;
    config.seed = 123;

    Grid grid2 = GridGenerator::generate(config);

    // Count generation capacity and total demand
    double totalGen = 0.0, totalDemand = 0.0;
    for (const auto& node : grid2.getNodes()) {
        if (node.type == NodeType::GENERATOR) totalGen += node.capacityMW;
        if (node.type == NodeType::LOAD) totalDemand += node.capacityMW;
    }

    std::cout << "Grid: " << grid2.nodeCount() << " nodes, " << grid2.edgeCount() << " edges\n";
    std::cout << "Total generation capacity: " << std::fixed << std::setprecision(2)
              << totalGen << " MW\n";
    std::cout << "Total demand: " << std::fixed << std::setprecision(2)
              << totalDemand << " MW\n";
    std::cout << "Surplus/Deficit: " << std::fixed << std::setprecision(2)
              << (totalGen - totalDemand) << " MW\n\n";

    MCMFResult result2 = MinCostMaxFlow::compute(grid2);

    std::cout << "Total flow delivered: " << std::fixed << std::setprecision(2)
              << result2.totalFlow << " MW\n";
    std::cout << "Total cost: " << std::fixed << std::setprecision(2) << result2.totalCost << "\n";

    std::cout << "\nEdge flows (first 15):\n";
    int printCount = std::min(15, static_cast<int>(result2.edgeFlows.size()));
    for (int i = 0; i < printCount; ++i) {
        const Edge& edge = grid2.getEdge(i);
        double cap = edge.capacityMW;
        double flow = result2.edgeFlows[i];
        double pct = (cap > 0) ? (flow / cap * 100.0) : 0.0;
        std::cout << "  Edge " << i << " (" << edge.from << " -> " << edge.to
                  << "): " << std::fixed << std::setprecision(2)
                  << flow << " / " << cap << " MW (" << std::setprecision(1)
                  << pct << "%)\n";
    }

    std::cout << "\nUnmet demand: ";
    if (result2.unmetDemand.empty()) {
        std::cout << "NONE\n";
    } else {
        std::cout << result2.unmetDemand.size() << " nodes: ";
        for (int id : result2.unmetDemand) {
            const Node& node = grid2.getNode(id);
            std::cout << "Node " << id << " (" << node.tierString() << ") ";
        }
        std::cout << "\n";
    }

    // Test 3: Insufficient generation (should leave unmet demand)
    std::cout << "\n\n=== Test 3: Insufficient generation ===\n\n";
    GeneratorConfig config3;
    config3.totalNodes = 10;
    config3.generatorRatio = 0.1; // only 10% generators
    config3.substationRatio = 0.2;
    config3.redundancyFactor = 2;
    config3.seed = 456;

    Grid grid3 = GridGenerator::generate(config3);

    double gen3 = 0.0, dem3 = 0.0;
    for (const auto& node : grid3.getNodes()) {
        if (node.type == NodeType::GENERATOR) gen3 += node.capacityMW;
        if (node.type == NodeType::LOAD) dem3 += node.capacityMW;
    }

    std::cout << "Generation: " << std::fixed << std::setprecision(2) << gen3 << " MW\n";
    std::cout << "Demand: " << std::fixed << std::setprecision(2) << dem3 << " MW\n";

    MCMFResult result3 = MinCostMaxFlow::compute(grid3);
    std::cout << "Flow delivered: " << std::fixed << std::setprecision(2)
              << result3.totalFlow << " MW\n";
    std::cout << "Unmet demand nodes: " << result3.unmetDemand.size() << "\n";
    for (int id : result3.unmetDemand) {
        const Node& node = grid3.getNode(id);
        std::cout << "  Node " << id << " | Tier: " << node.tierString()
                  << " | Demand: " << node.capacityMW << " MW\n";
    }

    std::cout << "\n=== Min-Cost Max-Flow Test Complete ===\n";
    return 0;
}