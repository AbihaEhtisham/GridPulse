#pragma once

namespace gridpulse {

struct Edge {
    int id;
    int from;          // source node ID
    int to;            // destination node ID
    double capacityMW; // max flow this line can carry
    double currentFlowMW;
    double resistance; // used as Dijkstra edge weight
    bool tripped;      // true if line is down/failed

    // Default constructor
    Edge();

    // Parameterized constructor
    Edge(int id, int from, int to, double capacityMW, double resistance);

    // Utility: check if overloaded
    bool isOverloaded(double tolerance = 0.01) const;
};

} // namespace gridpulse