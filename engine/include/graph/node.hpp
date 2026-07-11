#pragma once

#include <string>

namespace gridpulse {

enum class NodeType {
    GENERATOR,
    SUBSTATION,
    LOAD
};

enum class CriticalityTier {
    CRITICAL = 1,
    COMMERCIAL = 2,
    RESIDENTIAL = 3
};

struct Node {
    int id;
    NodeType type;
    double capacityMW;        // max output (generators) or throughput (substations)
    double currentLoadMW;     // current load or generation
    CriticalityTier tier;     // priority for load shedding (LOAD nodes only)
    double x, y;              // spatial coordinates for visualization

    // Default constructor
    Node();

    // Parameterized constructor
    Node(int id, NodeType type, double capacityMW, CriticalityTier tier, double x, double y);

    // Utility: return type as string
    std::string typeString() const;

    // Utility: return tier as string
    std::string tierString() const;
};

} // namespace gridpulse