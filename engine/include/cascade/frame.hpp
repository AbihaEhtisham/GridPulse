#pragma once

#include <vector>
#include <string>
#include "graph/grid.hpp"

namespace gridpulse {

struct Frame {
    int frameNumber;
    std::vector<double> nodeLoads;     // currentLoadMW for each node
    std::vector<double> edgeFlows;     // currentFlowMW for each edge
    std::vector<bool> edgeTripped;     // tripped status for each edge
    std::vector<int> islandIds;        // island assignment for each node
    std::vector<bool> nodeShed;        // true if node load was shed this frame
    std::string eventDescription;      // what happened in this frame

    Frame() : frameNumber(0) {}
};

} // namespace gridpulse