#include "graph/edge.hpp"

namespace gridpulse {

Edge::Edge()
    : id(-1), from(-1), to(-1), capacityMW(0.0), currentFlowMW(0.0),
      resistance(0.0), tripped(false) {}

Edge::Edge(int id, int from, int to, double capacityMW, double resistance)
    : id(id), from(from), to(to), capacityMW(capacityMW), currentFlowMW(0.0),
      resistance(resistance), tripped(false) {}

bool Edge::isOverloaded(double tolerance) const {
    if (tripped) return false;
    return currentFlowMW > capacityMW + tolerance;
}

} // namespace gridpulse