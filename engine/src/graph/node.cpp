#include "graph/node.hpp"

namespace gridpulse {

Node::Node()
    : id(-1), type(NodeType::LOAD), capacityMW(0.0), currentLoadMW(0.0),
      tier(CriticalityTier::RESIDENTIAL), x(0.0), y(0.0) {}

Node::Node(int id, NodeType type, double capacityMW, CriticalityTier tier, double x, double y)
    : id(id), type(type), capacityMW(capacityMW), currentLoadMW(0.0),
      tier(tier), x(x), y(y) {}

std::string Node::typeString() const {
    switch (type) {
        case NodeType::GENERATOR:  return "GENERATOR";
        case NodeType::SUBSTATION: return "SUBSTATION";
        case NodeType::LOAD:       return "LOAD";
        default:                   return "UNKNOWN";
    }
}

std::string Node::tierString() const {
    switch (tier) {
        case CriticalityTier::CRITICAL:    return "CRITICAL";
        case CriticalityTier::COMMERCIAL:  return "COMMERCIAL";
        case CriticalityTier::RESIDENTIAL: return "RESIDENTIAL";
        default:                           return "UNKNOWN";
    }
}

} // namespace gridpulse