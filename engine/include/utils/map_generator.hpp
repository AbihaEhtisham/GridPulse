#pragma once

#include "graph/grid.hpp"
#include <random>
#include <vector>
#include <string>

namespace gridpulse {

struct MapConfig {
    int mapWidth;           // pixels
    int mapHeight;          // pixels
    int numPowerPlants;     // generators on outskirts
    int numSubstations;     // transformer stations
    int numHospitals;       // critical loads
    int numCommercial;      // commercial loads
    int numResidential;     // residential loads
    int redundancyFactor;   // extra edges beyond minimum
    int seed;

    MapConfig()
        : mapWidth(1200), mapHeight(800)
        , numPowerPlants(3), numSubstations(5)
        , numHospitals(2), numCommercial(8)
        , numResidential(20), redundancyFactor(4)
        , seed(42) {}
};

class MapGridGenerator {
public:
    explicit MapGridGenerator(const MapConfig& config);

    // Generate a city-style grid with proper layout
    Grid generate();

    // Static convenience
    static Grid generate(const MapConfig& config);

private:
    MapConfig config_;
    std::mt19937 rng_;

    // Place power plants on outskirts
    void placePowerPlants(Grid& grid);

    // Place substations in a grid pattern
    void placeSubstations(Grid& grid);

    // Place hospital/emergency buildings in city center
    void placeCriticalLoads(Grid& grid);

    // Place commercial buildings
    void placeCommercialLoads(Grid& grid);

    // Place residential houses in neighborhoods
    void placeResidentialLoads(Grid& grid);

    // Connect everything with transmission/distribution lines
    void buildConnections(Grid& grid);

    // Connect a node to nearest substation
    void connectToNearestSubstation(Grid& grid, int nodeId, const std::vector<int>& substationIds);

    // Connect substations in a ring/mesh
    void connectSubstations(Grid& grid, const std::vector<int>& substationIds);

    // Connect power plants to nearest substations
    void connectPowerPlants(Grid& grid, const std::vector<int>& plantIds, const std::vector<int>& substationIds);

    // Add redundant connections for N-1 resilience
    void addRedundancy(Grid& grid);

    double distance(int n1, int n2, const Grid& grid) const;
    double randomDouble(double min, double max);
    int randomInt(int min, int max);
};

} // namespace gridpulse