#pragma once

#include "simclock/event_queue.hpp"
#include "graph/grid.hpp"
#include "cascade/cascade_loop.hpp"
#include "cascade/frame.hpp"
#include <vector>
#include <random>

namespace gridpulse {

struct StormConfig {
    double duration;            // Total storm duration in sim-seconds
    double intensity;           // 0.0 to 1.0, affects failure probability
    int numInitialFailures;     // Lines that fail when storm hits
    double overloadCheckInterval; // How often to check for cascading overloads
    int seed;
    double playerBudget;        // Budget for reinforcements

    StormConfig()
        : duration(120.0), intensity(0.5), numInitialFailures(3),
          overloadCheckInterval(5.0), seed(42), playerBudget(100.0) {}
};

struct StormFrame {
    SimTime timestamp;
    Frame gridFrame;
    std::string eventDescription;
    double gridHealth;          // 0.0 to 1.0
    double criticalLoadProtected; // 0.0 to 1.0
    int linesFailed;
    int nodesFailed;
};

struct StormResult {
    std::vector<StormFrame> timeline;
    double finalGridHealth;
    double finalCriticalProtected;
    int totalLinesFailed;
    int totalNodesFailed;
    bool gridSurvived;
    double playerBudgetRemaining;
};

class StormSimulator {
public:
    explicit StormSimulator(const StormConfig& config);

    // Initialize the storm on a grid
    void initialize(const Grid& grid);

    // Run the entire storm simulation
    StormResult runFull();

    // Run one event at a time (for live streaming)
    StormFrame stepNext();

    // Check if storm is finished
    bool isFinished() const;

    // Player actions during storm
    void playerCutEdge(int edgeId);
    void playerSpikeDemand(int nodeId, double amount);
    bool playerReinforceLine(int edgeId);  // Returns true if affordable

    // Get current grid state
    const Grid& getGrid() const { return grid_; }

    // Get event queue (for debug display)
    const EventQueue& getQueue() const { return queue_; }

private:
    StormConfig config_;
    Grid grid_;
    EventQueue queue_;
    std::mt19937 rng_;
    std::vector<StormFrame> timeline_;
    double gridHealth_;
    double criticalProtected_;
    int linesFailed_;
    int nodesFailed_;
    double playerBudget_;

    // Internal event handlers
    void handleStormAdvance();
    void handleLineFailure(int edgeId);
    void handleOverloadCheck();
    void handleCascadeStep();
    void scheduleStormEvents();
    void updateMetrics();
    StormFrame captureFrame(const std::string& description);
    std::vector<int> getVulnerableEdges();
};

} // namespace gridpulse