#pragma once

#include <vector>
#include "graph/grid.hpp"
#include "cascade/frame.hpp"

namespace gridpulse {

enum class TriggerType {
    CUT_EDGE,        // trip a specific transmission line
    FAIL_NODE,       // disable a specific node (generator or substation)
    SPIKE_DEMAND     // double the demand at a specific load node
};

struct TriggerEvent {
    TriggerType type;
    int targetId;     // edge ID or node ID
    double value;     // for spike: new demand value (0 = use default doubling)
};

struct CascadeResult {
    std::vector<Frame> frames;        // all frames of the cascade
    int totalIterations;              // number of cascade iterations
    bool gridStabilized;              // true if cascade stopped naturally
    bool totalBlackout;               // true if all loads disconnected
    double finalDemandServed;         // MW of demand still served at end
    double initialDemand;             // MW of demand before trigger
    int totalEdgesTripped;            // how many edges tripped during cascade
    int totalNodesShed;               // how many load nodes were shed

    CascadeResult()
        : totalIterations(0), gridStabilized(false), totalBlackout(false),
          finalDemandServed(0.0), initialDemand(0.0),
          totalEdgesTripped(0), totalNodesShed(0) {}
};

class CascadeLoop {
public:
    // Run a cascade simulation
    static CascadeResult run(const Grid& grid, const TriggerEvent& trigger);

    // Maximum cascade iterations (safety limit)
    static constexpr int MAX_CASCADE_ITERATIONS = 100;

private:
    // Record current grid state as a frame
    static Frame recordFrame(
        int frameNum,
        const Grid& grid,
        const std::vector<double>& edgeFlows,
        const std::vector<int>& islandIds,
        const std::vector<bool>& shedNodes,
        const std::string& description
    );

    // Calculate total demand in a set of nodes
    static double calculateTotalDemand(const Grid& grid, const std::vector<int>& nodeIds);

    // Calculate total generation in a set of nodes
    static double calculateTotalGeneration(const Grid& grid, const std::vector<int>& nodeIds);
};

} // namespace gridpulse