#pragma once

#include "graph/grid.hpp"
#include <random>
#include <string>

namespace gridpulse {

struct GeneratorConfig {
    int totalNodes;          // total number of nodes in the grid
    double generatorRatio;   // fraction of nodes that are generators (0.0 to 1.0)
    double substationRatio;  // fraction of nodes that are substations (0.0 to 1.0)
    int redundancyFactor;    // number of extra edges beyond the spanning tree
    double canvasWidth;      // width of the spatial canvas for node coordinates
    double canvasHeight;     // height of the spatial canvas for node coordinates
    int seed;                // random seed for reproducibility

    GeneratorConfig()
        : totalNodes(40)
        , generatorRatio(0.15)
        , substationRatio(0.25)
        , redundancyFactor(5)
        , canvasWidth(800.0)
        , canvasHeight(600.0)
        , seed(42) {}
};

class GridGenerator {
public:
    explicit GridGenerator(const GeneratorConfig& config);

    // Generate a complete grid with guaranteed connectivity
    Grid generate();

    // Static convenience function
    static Grid generate(const GeneratorConfig& config);

private:
    GeneratorConfig config_;
    std::mt19937 rng_;

    // Place nodes at random positions on the canvas
    void placeNodes(Grid& grid);

    // Assign types to nodes based on configured ratios
    void assignNodeTypes(Grid& grid);

    // Assign criticality tiers to load nodes
    void assignCriticalityTiers(Grid& grid);

    // Build a spanning tree to guarantee connectivity
    void buildSpanningTree(Grid& grid, const std::vector<int>& nodeIds);

    // Add extra redundant edges
    void addRedundantEdges(Grid& grid, const std::vector<int>& nodeIds);

    // Generate random double in range
    double randomDouble(double min, double max);

    // Generate random int in range
    int randomInt(int min, int max);

    // Fisher-Yates shuffle
    void shuffle(std::vector<int>& vec);
};

// Serialize grid to JSON string
std::string gridToJson(const Grid& grid);

// Deserialize grid from JSON string
Grid gridFromJson(const std::string& json);

} // namespace gridpulse