#include "simclock/storm_simulator.hpp"
#include "flow/min_cost_max_flow.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iostream>

namespace gridpulse {

StormSimulator::StormSimulator(const StormConfig& config)
    : config_(config), rng_(config.seed), gridHealth_(1.0),
      criticalProtected_(1.0), linesFailed_(0), nodesFailed_(0),
      playerBudget_(config.playerBudget) {}

void StormSimulator::initialize(const Grid& grid) {
    grid_ = grid;
    gridHealth_ = 1.0;
    criticalProtected_ = 1.0;
    linesFailed_ = 0;
    nodesFailed_ = 0;
    playerBudget_ = config_.playerBudget;
    timeline_.clear();
    queue_.clear();

    // Compute initial flow
    auto flowResult = MinCostMaxFlow::compute(grid_);
    for (size_t i = 0; i < flowResult.edgeFlows.size(); ++i) {
        if (i < static_cast<size_t>(grid_.edgeCount())) {
            grid_.getEdge(static_cast<int>(i)).currentFlowMW = flowResult.edgeFlows[i];
        }
    }

    // Schedule storm events at regular intervals
    double interval = config_.duration / 15.0; // 15 events during the storm
    for (int i = 1; i <= 15; i++) {
        double t = i * interval;
        queue_.schedule(t, EventType::STORM_FRONT_ADVANCE, -1, 0.0,
                       "Storm wave " + std::to_string(i), 10);
    }

    // Schedule initial line failures early in the storm
    int edgesToFail = std::min(config_.numInitialFailures, std::max(1, grid_.edgeCount() / 4));
    for (int i = 0; i < edgesToFail; i++) {
        double failTime = 3.0 + i * 4.0;
        int edgeId = (i * 7 + 3) % grid_.edgeCount();
        queue_.schedule(failTime, EventType::LINE_FAILURE, edgeId, 0.0,
                       "Lightning strike on line " + std::to_string(edgeId), 5);
    }

    // Schedule overload checks
    for (double t = 2.0; t < config_.duration; t += 8.0) {
        queue_.schedule(t, EventType::OVERLOAD_CHECK, -1, 0.0, "Grid stress check", 3);
    }

    // Schedule storm end
    queue_.schedule(config_.duration, EventType::STORM_END, -1, 0.0,
                   "Storm system has passed", 0);

    // Record initial frame
    timeline_.push_back(captureFrame("Storm approaching the city..."));
}

StormResult StormSimulator::runFull() {
    while (!isFinished()) {
        stepNext();
    }
    StormResult result;
    result.timeline = timeline_;
    result.finalGridHealth = gridHealth_;
    result.finalCriticalProtected = criticalProtected_;
    result.totalLinesFailed = linesFailed_;
    result.totalNodesFailed = nodesFailed_;
    result.gridSurvived = gridHealth_ > 0.1;
    result.playerBudgetRemaining = playerBudget_;
    return result;
}

StormFrame StormSimulator::stepNext() {
    if (queue_.isEmpty()) {
        StormFrame sf;
        sf.timestamp = config_.duration;
        sf.eventDescription = "No more events";
        sf.gridHealth = gridHealth_;
        sf.criticalLoadProtected = criticalProtected_;
        sf.linesFailed = linesFailed_;
        sf.nodesFailed = nodesFailed_;
        return sf;
    }

    SimEvent event = queue_.pop();

    switch (event.type) {
        case EventType::STORM_FRONT_ADVANCE:
            handleStormAdvance();
            break;
        case EventType::LINE_FAILURE:
            handleLineFailure(event.targetId);
            break;
        case EventType::OVERLOAD_CHECK:
            handleOverloadCheck();
            break;
        case EventType::STORM_END:
            // Storm is over
            break;
        default:
            break;
    }

    updateMetrics();

    std::string desc = event.description;
    if (event.type == EventType::LINE_FAILURE) {
        desc = event.description + " - Grid stress increasing";
    }

    return captureFrame(desc);
}

void StormSimulator::handleStormAdvance() {
    // Randomly fail some edges as storm intensifies
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    std::vector<int> candidates;
    
    for (int i = 0; i < grid_.edgeCount(); i++) {
        if (!grid_.getEdge(i).tripped) {
            candidates.push_back(i);
        }
    }

    int numToFail = std::max(0, static_cast<int>(candidates.size() * config_.intensity * 0.1));
    std::shuffle(candidates.begin(), candidates.end(), rng_);

    for (int i = 0; i < numToFail && i < static_cast<int>(candidates.size()); i++) {
        int edgeId = candidates[i];
        double failTime = queue_.currentTime() + 0.1 + (i * 0.05);
        queue_.schedule(failTime, EventType::LINE_FAILURE, edgeId, 0.0,
                       "Storm damages transmission line " + std::to_string(edgeId), 6);
    }
}

void StormSimulator::handleLineFailure(int edgeId) {
    if (edgeId < 0 || edgeId >= grid_.edgeCount()) return;

    Edge& edge = grid_.getEdge(edgeId);
    if (!edge.tripped) {
        edge.tripped = true;
        linesFailed_++;

        // Recompute flow after failure
        auto flowResult = MinCostMaxFlow::compute(grid_);
        for (size_t i = 0; i < flowResult.edgeFlows.size(); i++) {
            if (i < static_cast<size_t>(grid_.edgeCount())) {
                grid_.getEdge(static_cast<int>(i)).currentFlowMW = flowResult.edgeFlows[i];
            }
        }
    }
}

void StormSimulator::handleOverloadCheck() {
    std::vector<int> newlyOverloaded;
    for (int i = 0; i < grid_.edgeCount(); i++) {
        Edge& edge = grid_.getEdge(i);
        if (!edge.tripped && edge.isOverloaded()) {
            newlyOverloaded.push_back(i);
        }
    }

    for (int edgeId : newlyOverloaded) {
        grid_.getEdge(edgeId).tripped = true;
        linesFailed_++;
    }

    if (!newlyOverloaded.empty()) {
        auto flowResult = MinCostMaxFlow::compute(grid_);
        for (size_t i = 0; i < flowResult.edgeFlows.size(); i++) {
            if (i < static_cast<size_t>(grid_.edgeCount())) {
                grid_.getEdge(static_cast<int>(i)).currentFlowMW = flowResult.edgeFlows[i];
            }
        }
    }
}

void StormSimulator::updateMetrics() {
    double totalDemand = 0.0;
    double servedDemand = 0.0;
    double criticalDemand = 0.0;
    double criticalServed = 0.0;

    for (const auto& node : grid_.getNodes()) {
        if (node.type == NodeType::LOAD) {
            totalDemand += node.capacityMW;
            // Check if node can be reached from a generator
            bool hasPower = false;
            for (int edgeId : grid_.getIncidentEdges(node.id)) {
                const Edge& e = grid_.getEdge(edgeId);
                if (!e.tripped && e.currentFlowMW > 0.1) {
                    hasPower = true;
                    break;
                }
            }
            if (hasPower || grid_.isConnected()) {
                servedDemand += node.capacityMW;
                if (node.tier == CriticalityTier::CRITICAL) {
                    criticalServed += node.capacityMW;
                }
            }
            if (node.tier == CriticalityTier::CRITICAL) {
                criticalDemand += node.capacityMW;
            }
        }
    }

    gridHealth_ = (totalDemand > 0) ? servedDemand / totalDemand : 0.0;
    criticalProtected_ = (criticalDemand > 0) ? criticalServed / criticalDemand : 0.0;

    // Clamp values
    gridHealth_ = std::max(0.0, std::min(1.0, gridHealth_));
    criticalProtected_ = std::max(0.0, std::min(1.0, criticalProtected_));
}

StormFrame StormSimulator::captureFrame(const std::string& description) {
    StormFrame sf;
    sf.timestamp = queue_.currentTime();
    sf.eventDescription = description;
    sf.gridHealth = gridHealth_;
    sf.criticalLoadProtected = criticalProtected_;
    sf.linesFailed = linesFailed_;
    sf.nodesFailed = nodesFailed_;

    sf.gridFrame.frameNumber = static_cast<int>(timeline_.size());
    sf.gridFrame.eventDescription = description;
    for (const auto& node : grid_.getNodes()) {
        sf.gridFrame.nodeLoads.push_back(node.currentLoadMW);
        sf.gridFrame.islandIds.push_back(0);
        sf.gridFrame.nodeShed.push_back(false);
    }
    for (const auto& edge : grid_.getEdges()) {
        sf.gridFrame.edgeFlows.push_back(edge.currentFlowMW);
        sf.gridFrame.edgeTripped.push_back(edge.tripped);
    }

    timeline_.push_back(sf);
    return sf;
}

bool StormSimulator::isFinished() const {
    return queue_.isEmpty();
}

void StormSimulator::playerCutEdge(int edgeId) {
    if (edgeId < 0 || edgeId >= grid_.edgeCount()) return;
    if (!grid_.getEdge(edgeId).tripped) {
        grid_.getEdge(edgeId).tripped = true;
        linesFailed_++;
        auto flowResult = MinCostMaxFlow::compute(grid_);
        for (size_t i = 0; i < flowResult.edgeFlows.size(); i++) {
            if (i < static_cast<size_t>(grid_.edgeCount())) {
                grid_.getEdge(static_cast<int>(i)).currentFlowMW = flowResult.edgeFlows[i];
            }
        }
        timeline_.push_back(captureFrame("Player cut line " + std::to_string(edgeId)));
    }
}

void StormSimulator::playerSpikeDemand(int nodeId, double amount) {
    if (nodeId < 0 || nodeId >= grid_.nodeCount()) return;
    grid_.getNode(nodeId).capacityMW += amount;
    auto flowResult = MinCostMaxFlow::compute(grid_);
    for (size_t i = 0; i < flowResult.edgeFlows.size(); i++) {
        if (i < static_cast<size_t>(grid_.edgeCount())) {
            grid_.getEdge(static_cast<int>(i)).currentFlowMW = flowResult.edgeFlows[i];
        }
    }
}

bool StormSimulator::playerReinforceLine(int edgeId) {
    const double REINFORCE_COST = 20.0;
    if (playerBudget_ < REINFORCE_COST) return false;
    if (edgeId < 0 || edgeId >= grid_.edgeCount()) return false;

    playerBudget_ -= REINFORCE_COST;
    Edge& edge = grid_.getEdge(edgeId);
    edge.capacityMW *= 1.5;
    return true;
}

} // namespace gridpulse
