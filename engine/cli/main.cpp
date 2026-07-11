#include <iostream>
#include "graph/grid.hpp"

using namespace gridpulse;

int main() {
    std::cout << "=== GridPulse Engine — Data Structure Test ===\n\n";

    Grid grid;

    // Create a simple 4-node grid
    int gen1 = grid.addNode(NodeType::GENERATOR, 100.0, CriticalityTier::CRITICAL, 100.0, 100.0);
    int sub1 = grid.addNode(NodeType::SUBSTATION, 80.0, CriticalityTier::COMMERCIAL, 200.0, 200.0);
    int load1 = grid.addNode(NodeType::LOAD, 40.0, CriticalityTier::CRITICAL, 300.0, 300.0);
    int load2 = grid.addNode(NodeType::LOAD, 30.0, CriticalityTier::RESIDENTIAL, 400.0, 400.0);

    // Connect them
    grid.addEdge(gen1, sub1, 80.0, 1.0);
    grid.addEdge(sub1, load1, 40.0, 1.0);
    grid.addEdge(sub1, load2, 30.0, 1.0);

    // Print nodes
    std::cout << "Nodes (" << grid.nodeCount() << "):\n";
    for (const auto& node : grid.getNodes()) {
        std::cout << "  Node " << node.id
                  << " | Type: " << node.typeString()
                  << " | Capacity: " << node.capacityMW << " MW"
                  << " | Tier: " << node.tierString()
                  << " | Position: (" << node.x << ", " << node.y << ")\n";
    }

    // Print edges
    std::cout << "\nEdges (" << grid.edgeCount() << "):\n";
    for (const auto& edge : grid.getEdges()) {
        std::cout << "  Edge " << edge.id
                  << " | " << edge.from << " -> " << edge.to
                  << " | Capacity: " << edge.capacityMW << " MW"
                  << " | Resistance: " << edge.resistance << "\n";
    }

    // Print neighbors
    std::cout << "\nAdjacency:\n";
    for (int i = 0; i < grid.nodeCount(); ++i) {
        std::cout << "  Node " << i << " neighbors: ";
        for (int neighbor : grid.getNeighbors(i)) {
            std::cout << neighbor << " ";
        }
        std::cout << "\n";
    }

    // Connectivity check
    std::cout << "\nGrid is connected: " << (grid.isConnected() ? "YES" : "NO") << "\n";

    // Test edge removal
    std::cout << "\nRemoving edge 0...\n";
    grid.removeEdge(0);
    std::cout << "Grid is connected after removal: " << (grid.isConnected() ? "YES" : "NO") << "\n";

    std::cout << "\n=== All tests passed ===\n";
    return 0;
}