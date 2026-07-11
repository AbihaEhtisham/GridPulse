#include <iostream>
#include <fstream>
#include "graph/grid.hpp"
#include "utils/grid_generator.hpp"

using namespace gridpulse;

int main() {
    std::cout << "=== GridPulse Engine — Grid Generator Test ===\n\n";

    // Configure generator
    GeneratorConfig config;
    config.totalNodes = 30;
    config.generatorRatio = 0.15;
    config.substationRatio = 0.25;
    config.redundancyFactor = 4;
    config.seed = 42;

    // Generate grid
    Grid grid = GridGenerator::generate(config);

    // Print summary
    int genCount = 0, subCount = 0, loadCount = 0;
    int critCount = 0, commCount = 0, resCount = 0;
    for (const auto& node : grid.getNodes()) {
        switch (node.type) {
            case NodeType::GENERATOR:  ++genCount; break;
            case NodeType::SUBSTATION: ++subCount; break;
            case NodeType::LOAD:       ++loadCount; break;
        }
        if (node.type == NodeType::LOAD) {
            switch (node.tier) {
                case CriticalityTier::CRITICAL:    ++critCount; break;
                case CriticalityTier::COMMERCIAL:  ++commCount; break;
                case CriticalityTier::RESIDENTIAL: ++resCount; break;
            }
        }
    }

    std::cout << "Grid generated with:\n";
    std::cout << "  Total nodes: " << grid.nodeCount() << "\n";
    std::cout << "  Generators:  " << genCount << "\n";
    std::cout << "  Substations: " << subCount << "\n";
    std::cout << "  Loads:       " << loadCount << "\n";
    std::cout << "    Critical:   " << critCount << "\n";
    std::cout << "    Commercial: " << commCount << "\n";
    std::cout << "    Residential:" << resCount << "\n";
    std::cout << "  Total edges: " << grid.edgeCount() << "\n";
    std::cout << "  Connected:   " << (grid.isConnected() ? "YES" : "NO") << "\n\n";

    // Print first 10 nodes
    std::cout << "Sample nodes (first 10):\n";
    int printCount = std::min(10, grid.nodeCount());
    for (int i = 0; i < printCount; ++i) {
        const auto& node = grid.getNodes()[i];
        std::cout << "  Node " << node.id
                  << " | " << node.typeString()
                  << " | Cap: " << node.capacityMW << " MW"
                  << " | " << node.tierString()
                  << " | (" << node.x << ", " << node.y << ")\n";
    }

    // Print first 10 edges
    std::cout << "\nSample edges (first 10):\n";
    printCount = std::min(10, grid.edgeCount());
    for (int i = 0; i < printCount; ++i) {
        const auto& edge = grid.getEdges()[i];
        std::cout << "  Edge " << edge.id
                  << " | " << edge.from << " -> " << edge.to
                  << " | Cap: " << edge.capacityMW << " MW"
                  << " | Res: " << edge.resistance << "\n";
    }

    // Export to JSON
    std::string json = gridToJson(grid);
    std::ofstream outFile("generated_grid.json");
    outFile << json;
    outFile.close();
    std::cout << "\nGrid exported to 'generated_grid.json'\n";

    // Also print to console (first 30 lines)
    std::cout << "\nJSON preview (first 30 lines):\n";
    std::istringstream stream(json);
    std::string line;
    int lineCount = 0;
    while (std::getline(stream, line) && lineCount < 30) {
        std::cout << line << "\n";
        ++lineCount;
    }

    std::cout << "\n=== Grid Generator Test Complete ===\n";
    return 0;
}