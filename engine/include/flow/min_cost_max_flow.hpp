#pragma once

#include <vector>
#include <limits>
#include "graph/grid.hpp"

namespace gridpulse {

struct MCMFEdge {
    int from;              // source node in flow network
    int to;                // destination node in flow network
    double capacity;       // remaining capacity
    double cost;           // cost per unit of flow
    double flow;           // current flow on this edge
    int reverseEdgeIndex;  // index of reverse edge in adjacency list
    int originalEdgeId;    // corresponding Grid edge ID (-1 for artificial edges)

    MCMFEdge(int f, int t, double cap, double c, int rev, int orig = -1)
        : from(f), to(t), capacity(cap), cost(c), flow(0.0),
          reverseEdgeIndex(rev), originalEdgeId(orig) {}
};

struct MCMFResult {
    double totalFlow;       // total flow successfully delivered
    double totalCost;       // total cost of delivered flow
    std::vector<double> nodeSupply;  // final net supply at each node (+ = generation, - = demand)
    std::vector<double> edgeFlows;   // final flow on each original grid edge
    std::vector<int> unmetDemand;    // list of load node IDs with unmet demand
    bool success;

    MCMFResult() : totalFlow(0.0), totalCost(0.0), success(false) {}
};

class MinCostMaxFlow {
public:
    // Run MCMF on a grid
    // Generators are sources with supply = capacity
    // Loads are sinks with demand = capacity
    // Edge costs are based on resistance, modified by criticality tier
    static MCMFResult compute(const Grid& grid);

    // Run MCMF with custom supply/demand values (for islanded sub-grids)
    static MCMFResult computeWithSupply(
        const Grid& grid,
        const std::vector<double>& supply  // positive = generation, negative = demand
    );

private:
    // Build flow network from grid
    struct FlowNetwork {
        std::vector<std::vector<MCMFEdge>> adjacency; // adjacency list for flow network
        int superSource;  // ID of super-source node (-1 if none needed)
        int superSink;    // ID of super-sink node (-1 if none needed)
        int numNodes;     // total nodes in flow network
        int numGridNodes; // original grid nodes
    };

    static FlowNetwork buildNetwork(
        const Grid& grid,
        const std::vector<double>& supply
    );

    // Compute cost weight for serving a load through an edge
    static double computeEdgeCost(const Grid& grid, int edgeId, int targetLoadId);

    // SPFA for finding shortest augmenting path in residual graph
    static bool findAugmentingPath(
        const FlowNetwork& network,
        int source,
        int sink,
        std::vector<int>& predecessorNode,
        std::vector<int>& predecessorEdge
    );

    // Push flow along the found augmenting path
    static double augment(
        FlowNetwork& network,
        int source,
        int sink,
        const std::vector<int>& predecessorNode,
        const std::vector<int>& predecessorEdge
    );
};

} // namespace gridpulse