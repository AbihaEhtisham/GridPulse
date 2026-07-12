#include "simclock/storm_simulator.hpp"
#include "flow/min_cost_max_flow.hpp"
#include "cascade/cascade_loop.hpp"
#include <algorithm>
#include <cmath>
#include <sstream>

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
        grid_.getEdge(static_cast<int>(i)).currentFlowMW = flowResult.edgeFlows[i];
    }

    // Schedule storm events
    scheduleStormEvents();

    // Record initial frame
    timeline_.push_back(captureFrame("Storm approaching..."));
}

void StormSimulator::scheduleStormEvents() {
    // Schedule storm front advances
    for (double t = 0; t < config_.duration; t += 10.0) {
        queue_.schedule(t, EventType::STORM_FRONT_ADVANCE, -1, 
                        config_.intensity, "Storm front advancing", 10);
    }

    // Schedule initial line failures
    auto vulnerable = getVulnerableEdges();
    std::shuffle(vulnerable.begin(), vulnerable.end(), rng_);
    int numFail = std::min(config_.numInitialFailures, static_cast<int>(vulnerable.size()));
    for (int i = 0; i < numFail; ++i) {
        double failTime = 5.0 + (i * 3.0) + (rand() % 100) / 100.0 * 5.0;
        queue_.schedule(failTime, EventType::LINE_FAILURE, vulnerable[i],
                        0.0, "Storm damages transmission line", 5);
    }

    // Schedule periodic overload checks
    for (double t = 1.0; t < config_.duration; t += config_.overloadCheckInterval) {
        queue_.schedule(t, EventType::OVERLOAD_CHECK, -1, 0.0, "Checking for overloads", 3);
    }

    // Schedule storm end
    queue_.schedule(config_.duration, EventType::STORM_END, -1, 0.0, "Storm passes", 0);
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
        return captureFrame("No more events");
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
            break;
        default:
            break;
    }

    updateMetrics();
    return captureFrame(event.description);
}

void StormSimulator::handleStormAdvance() {
    // Storm intensity increases failure probability for exposed edges
    auto vulnerable = getVulnerableEdges();
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    
    for (int edgeId : vulnerable) {
        double failChance = config_.intensity * 0.15;
        if (dist(rng_) < failChance) {
            queue_.schedule(queue_.currentTime() + 0.5, EventType::LINE_FAILURE,
                           edgeId, 0.0, "Storm surge damages line", 6);
        }
    }
}

void StormSimulator::handleLineFailure(int edgeId) {
    if (edgeId < 0 || edgeId >= grid_.edgeCount()) return;
    
    Edge& edge = grid_.getEdge(edgeId);
    if (!edge.tripped) {
        edge.tripped = true;
        linesFailed_++;

        // Recompute flow
        auto flowResult = MinCostMaxFlow::compute(grid_);
        for (size_t i = 0; i < flowResult.edgeFlows.size(); ++i) {
            grid_.getEdge(static_cast<int>(i)).currentFlowMW = flowResult.edgeFlows[i];
        }

        // Schedule immediate overload check
        queue_.schedule(queue_.currentTime() + 0.1, EventType::OVERLOAD_CHECK,
                       -1, 0.0, "Immediate overload check after failure", 2);
    }
}

void StormSimulator::handleOverloadCheck() {
    std::vector<int> newlyOverloaded;
    for (int i = 0; i < grid_.edgeCount(); ++i) {
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
        for (size_t i = 0; i < flowResult.edgeFlows.size(); ++i) {
            grid_.getEdge(static_cast<int>(i)).currentFlowMW = flowResult.edgeFlows[i];
        }
    }
}

void StormSimulator::updateMetrics() {
    // Calculate grid health: fraction of demand being served
    double totalDemand = 0.0;
    double servedDemand = 0.0;
    double criticalDemand = 0.0;
    double criticalServed = 0.0;

    for (const auto& node : grid_.getNodes()) {
        if (node.type == NodeType::LOAD) {
            totalDemand += node.capacityMW;
            servedDemand += node.currentLoadMW;
            if (node.tier == CriticalityTier::CRITICAL) {
                criticalDemand += node.capacityMW;
                criticalServed += node.currentLoadMW;
            }
        }
    }

    gridHealth_ = (totalDemand > 0) ? servedDemand / totalDemand : 0.0;
    criticalProtected_ = (criticalDemand > 0) ? criticalServed / criticalDemand : 0.0;
}

StormFrame StormSimulator::captureFrame(const std::string& description) {
    StormFrame sf;
    sf.timestamp = queue_.currentTime();
    sf.eventDescription = description;
    sf.gridHealth = gridHealth_;
    sf.criticalLoadProtected = criticalProtected_;
    sf.linesFailed = linesFailed_;
    sf.nodesFailed = nodesFailed_;

    // Record grid state
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
    if (queue_.isEmpty()) return true;
    return queue_.peek().type == EventType::STORM_END && queue_.size() == 1;
}

void StormSimulator::playerCutEdge(int edgeId) {
    if (edgeId < 0 || edgeId >= grid_.edgeCount()) return;
    queue_.schedule(queue_.currentTime() + 0.01, EventType::PLAYER_CUT_EDGE,
                   edgeId, 0.0, "Player cut transmission line", 0);
    grid_.getEdge(edgeId).tripped = true;
    linesFailed_++;
}

void StormSimulator::playerSpikeDemand(int nodeId, double amount) {
    if (nodeId < 0 || nodeId >= grid_.nodeCount()) return;
    grid_.getNode(nodeId).capacityMW += amount;
}

bool StormSimulator::playerReinforceLine(int edgeId) {
    const double REINFORCE_COST = 20.0;
    if (playerBudget_ < REINFORCE_COST) return false;
    if (edgeId < 0 || edgeId >= grid_.edgeCount()) return false;

    playerBudget_ -= REINFORCE_COST;
    Edge& edge = grid_.getEdge(edgeId);
    edge.capacityMW *= 1.5; // 50% capacity boost
    return true;
}

std::vector<int> StormSimulator::getVulnerableEdges() {
    std::vector<int> vulnerable;
    for (int i = 0; i < grid_.edgeCount(); ++i) {
        const Edge& edge = grid_.getEdge(i);
        if (!edge.tripped) {
            vulnerable.push_back(i);
        }
    }
    return vulnerable;
}

} // namespace gridpulse