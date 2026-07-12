#pragma once

#include <queue>
#include <vector>
#include <functional>
#include <string>
#include <memory>

namespace gridpulse {

// Timestamp in simulation seconds
using SimTime = double;

// Types of events in the simulation
enum class EventType {
    STORM_FRONT_ADVANCE,    // Storm wave moves forward
    LINE_FAILURE,           // A transmission line fails due to storm
    NODE_FAILURE,           // A generator/substation fails
    DEMAND_SPIKE,           // Sudden demand increase
    OVERLOAD_CHECK,         // Check all lines for overloads
    PLAYER_CUT_EDGE,        // Player manually cuts a line
    PLAYER_SPIKE_DEMAND,    // Player spikes demand
    PLAYER_REINFORCE,       // Player reinforces a line
    CASCADE_STEP,           // One iteration of cascade propagation
    REPAIR_CREW_ARRIVE,     // Repair crew reaches a damage site
    REPAIR_COMPLETE,        // A repair is finished
    STORM_END,              // Storm phase ends
};

struct SimEvent {
    SimTime timestamp;
    EventType type;
    int targetId;           // Node or edge ID
    double value;           // Additional data (demand amount, damage, etc.)
    std::string description;
    int priority;           // Lower = higher priority (for tie-breaking)

    // For priority queue: earlier timestamp = higher priority
    bool operator>(const SimEvent& other) const {
        if (std::abs(timestamp - other.timestamp) > 1e-9) {
            return timestamp > other.timestamp;
        }
        return priority > other.priority;
    }
};

class EventQueue {
public:
    EventQueue() = default;

    // Schedule an event
    void schedule(const SimEvent& event);

    // Schedule with individual parameters
    void schedule(SimTime time, EventType type, int targetId, 
                  double value, const std::string& desc, int priority = 0);

    // Get the next event (does not remove)
    const SimEvent& peek() const;

    // Remove and return the next event
    SimEvent pop();

    // Check if queue is empty
    bool isEmpty() const;

    // Number of pending events
    size_t size() const;

    // Get current simulation time
    SimTime currentTime() const;

    // Advance simulation time
    void setCurrentTime(SimTime time);

    // Clear all events
    void clear();

private:
    std::priority_queue<SimEvent, std::vector<SimEvent>, std::greater<SimEvent>> queue_;
    SimTime currentTime_ = 0.0;
};

} // namespace gridpulse