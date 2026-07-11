#include "cascade/cascade_loop.hpp"
#include "cascade/union_find.hpp"
#include "cascade/load_shed_heap.hpp"
#include "flow/min_cost_max_flow.hpp"
#include <algorithm>
#include <iostream>
#include <cmath>

namespace gridpulse {

CascadeResult CascadeLoop::run(const Grid& grid, const TriggerEvent& trigger) {
    CascadeResult result;

    // Create a mutable copy of the grid
    Grid simGrid = grid;

    // Calculate initial demand
    for (const auto& node : simGrid.getNodes()) {
        if (node.type == NodeType::LOAD) {
            result.initialDemand += node.capacityMW;
        }
    }

    // ===== Step 1: Apply trigger =====
    std::string triggerDesc;
    if (trigger.type == TriggerType::CUT_EDGE) {
        simGrid.getEdge(trigger.targetId).tripped = true;
        triggerDesc = "Edge " + std::to_string(trigger.targetId) + " tripped";
        result.totalEdgesTripped = 1;
    } else if (trigger.type == TriggerType::FAIL_NODE) {
        simGrid.getNode(trigger.targetId).capacityMW = 0.0;
        // Trip all incident edges
        for (int edgeId : simGrid.getIncidentEdges(trigger.targetId)) {
            simGrid.getEdge(edgeId).tripped = true;
            result.totalEdgesTripped++;
        }
        triggerDesc = "Node " + std::to_string(trigger.targetId) + " failed";
    } else if (trigger.type == TriggerType::SPIKE_DEMAND) {
        double newDemand = (trigger.value > 0) ? trigger.value
                                                : simGrid.getNode(trigger.targetId).capacityMW * 2.0;
        simGrid.getNode(trigger.targetId).capacityMW = newDemand;
        triggerDesc = "Demand spiked at Node " + std::to_string(trigger.targetId);
    }

    // Record frame 0 (post-trigger, pre-recomputation)
    std::vector<double> initialFlows(simGrid.edgeCount(), 0.0);
    std::vector<int> initialIslands(simGrid.nodeCount(), 0);
    std::vector<bool> initialShed(simGrid.nodeCount(), false);
    result.frames.push_back(recordFrame(0, simGrid, initialFlows, initialIslands, initialShed, triggerDesc));

    int iteration = 0;
    bool stabilized = false;

    // ===== Step 2: Cascade loop =====
    while (iteration < MAX_CASCADE_ITERATIONS && !stabilized) {
        ++iteration;

        // Run MCMF
        MCMFResult flowResult = MinCostMaxFlow::compute(simGrid);

        // Update edge flows on simGrid
        for (size_t i = 0; i < flowResult.edgeFlows.size(); ++i) {
            simGrid.getEdge(static_cast<int>(i)).currentFlowMW = flowResult.edgeFlows[i];
        }

        // ===== Step 3: Scan for overloads =====
        std::vector<int> newlyTripped;
        for (int i = 0; i < simGrid.edgeCount(); ++i) {
            Edge& edge = simGrid.getEdge(i);
            if (!edge.tripped && edge.isOverloaded()) {
                edge.tripped = true;
                newlyTripped.push_back(edge.id);
            }
        }

        // ===== Step 4: If no new trips, stabilize =====
        if (newlyTripped.empty()) {
            stabilized = true;

            // Record final frame
            std::vector<int> finalIslands(simGrid.nodeCount(), 0);
            std::vector<bool> finalShed(simGrid.nodeCount(), false);
            result.frames.push_back(recordFrame(
                iteration, simGrid, flowResult.edgeFlows,
                finalIslands, finalShed,
                "Grid stabilized — no new overloads"
            ));
            break;
        }

        result.totalEdgesTripped += static_cast<int>(newlyTripped.size());

        std::string tripDesc = "Overloaded: ";
        for (size_t i = 0; i < newlyTripped.size(); ++i) {
            tripDesc += "Edge " + std::to_string(newlyTripped[i]);
            if (i < newlyTripped.size() - 1) tripDesc += ", ";
        }
        tripDesc += " tripped";

        // ===== Step 5: Union-Find island detection =====
        UnionFind uf(simGrid.nodeCount());
        for (const auto& edge : simGrid.getEdges()) {
            if (!edge.tripped) {
                uf.unionSets(edge.from, edge.to);
            }
        }

        auto components = uf.getComponents();
        std::vector<int> islandIds(simGrid.nodeCount(), -1);
        for (size_t i = 0; i < components.size(); ++i) {
            for (int nodeId : components[i]) {
                islandIds[nodeId] = static_cast<int>(i);
            }
        }

        std::vector<bool> shedNodes(simGrid.nodeCount(), false);

        // ===== Step 6: Per-island load shedding =====
        for (const auto& island : components) {
            double islandGen = calculateTotalGeneration(simGrid, island);
            double islandDemand = calculateTotalDemand(simGrid, island);

            if (islandDemand > islandGen + 1e-6) {
                // Need to shed load
                double deficit = islandDemand - islandGen;

                LoadShedHeap heap;
                heap.build(island, simGrid);

                double shedSoFar = 0.0;
                std::vector<int> shedThisIsland;

                while (!heap.isEmpty() && shedSoFar < deficit) {
                    ShedEntry entry = heap.pop();
                    simGrid.getNode(entry.nodeId).capacityMW = 0.0;
                    simGrid.getNode(entry.nodeId).currentLoadMW = 0.0;
                    shedSoFar += entry.loadMW;
                    shedNodes[entry.nodeId] = true;
                    shedThisIsland.push_back(entry.nodeId);
                }

                if (!shedThisIsland.empty()) {
                    result.totalNodesShed += static_cast<int>(shedThisIsland.size());
                }
            }
        }

        // Record frame
        result.frames.push_back(recordFrame(
            iteration, simGrid, flowResult.edgeFlows,
            islandIds, shedNodes, tripDesc
        ));

        // Check for total blackout
        double remainingDemand = 0.0;
        for (const auto& node : simGrid.getNodes()) {
            if (node.type == NodeType::LOAD) {
                remainingDemand += node.capacityMW;
            }
        }
        if (remainingDemand < 1e-6) {
            result.totalBlackout = true;
            break;
        }
    }

    // Calculate final demand served
    for (const auto& node : simGrid.getNodes()) {
        if (node.type == NodeType::LOAD) {
            result.finalDemandServed += node.capacityMW;
        }
    }

    result.totalIterations = iteration;
    result.gridStabilized = stabilized;

    return result;
}

// ==================== Private Helpers ====================

Frame CascadeLoop::recordFrame(
    int frameNum,
    const Grid& grid,
    const std::vector<double>& edgeFlows,
    const std::vector<int>& islandIds,
    const std::vector<bool>& shedNodes,
    const std::string& description
) {
    Frame frame;
    frame.frameNumber = frameNum;
    frame.eventDescription = description;

    for (const auto& node : grid.getNodes()) {
        frame.nodeLoads.push_back(node.currentLoadMW);
        frame.islandIds.push_back(
            (node.id < static_cast<int>(islandIds.size())) ? islandIds[node.id] : -1
        );
        frame.nodeShed.push_back(
            (node.id < static_cast<int>(shedNodes.size())) ? shedNodes[node.id] : false
        );
    }

    for (size_t i = 0; i < grid.getEdges().size(); ++i) {
        frame.edgeFlows.push_back(
            (i < edgeFlows.size()) ? edgeFlows[i] : 0.0
        );
        frame.edgeTripped.push_back(grid.getEdges()[i].tripped);
    }

    return frame;
}

double CascadeLoop::calculateTotalDemand(const Grid& grid, const std::vector<int>& nodeIds) {
    double total = 0.0;
    for (int id : nodeIds) {
        const Node& node = grid.getNode(id);
        if (node.type == NodeType::LOAD) {
            total += node.capacityMW;
        }
    }
    return total;
}

double CascadeLoop::calculateTotalGeneration(const Grid& grid, const std::vector<int>& nodeIds) {
    double total = 0.0;
    for (int id : nodeIds) {
        const Node& node = grid.getNode(id);
        if (node.type == NodeType::GENERATOR) {
            total += node.capacityMW;
        }
    }
    return total;
}

} // namespace gridpulse