#include "flow/min_cost_max_flow.hpp"
#include <queue>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <iostream>

namespace gridpulse {

// ==================== Public Interface ====================

MCMFResult MinCostMaxFlow::compute(const Grid& grid) {
    int n = grid.nodeCount();
    std::vector<double> supply(n, 0.0);

    for (const auto& node : grid.getNodes()) {
        if (node.type == NodeType::GENERATOR) {
            supply[node.id] = node.capacityMW;  // positive = available generation
        } else if (node.type == NodeType::LOAD) {
            supply[node.id] = -node.capacityMW; // negative = demand to satisfy
        }
        // Substations have zero supply (just pass-through)
    }

    return computeWithSupply(grid, supply);
}

MCMFResult MinCostMaxFlow::computeWithSupply(
    const Grid& grid,
    const std::vector<double>& supply
) {
    MCMFResult result;
    int n = grid.nodeCount();

    if (n == 0) {
        result.success = true;
        return result;
    }

    result.nodeSupply = supply;
    result.edgeFlows.assign(grid.edgeCount(), 0.0);

    // Build flow network
    FlowNetwork network = buildNetwork(grid, supply);

    // Keep running SPFA to find augmenting paths
    double totalFlow = 0.0;
    double totalCost = 0.0;
    int maxIterations = grid.edgeCount() * grid.nodeCount() * 2; // safety bound
    int iteration = 0;

    while (iteration < maxIterations) {
        ++iteration;

        std::vector<int> predecessorNode(network.numNodes, -1);
        std::vector<int> predecessorEdge(network.numNodes, -1);

        bool found = findAugmentingPath(
            network,
            network.superSource,
            network.superSink,
            predecessorNode,
            predecessorEdge
        );

        if (!found) break; // no more augmenting paths

        double flowAdded = augment(
            network,
            network.superSource,
            network.superSink,
            predecessorNode,
            predecessorEdge
        );

        if (flowAdded < 1e-9) break; // negligible flow

        totalFlow += flowAdded;
        ++iteration;
    }

    // Extract edge flows from the flow network
    for (int i = 0; i < n; ++i) {
        for (const auto& fEdge : network.adjacency[i]) {
            if (fEdge.originalEdgeId >= 0 && fEdge.flow > 0) {
                // Determine flow direction for original undirected edge
                const Edge& origEdge = grid.getEdge(fEdge.originalEdgeId);
                if (fEdge.from == origEdge.from && fEdge.to == origEdge.to) {
                    result.edgeFlows[fEdge.originalEdgeId] += fEdge.flow;
                } else {
                    result.edgeFlows[fEdge.originalEdgeId] -= fEdge.flow;
                }
            }
        }
    }

    // Take absolute values (for undirected edges, flow is magnitude)
    for (auto& flow : result.edgeFlows) {
        flow = std::abs(flow);
    }

    // Compute total cost
    for (int i = 0; i < n; ++i) {
        for (const auto& fEdge : network.adjacency[i]) {
            if (fEdge.originalEdgeId >= 0 && fEdge.flow > 0) {
                totalCost += fEdge.flow * fEdge.cost;
            }
        }
    }

    // Identify unmet demand
    for (int i = 0; i < n; ++i) {
        if (supply[i] < -1e-6) {
            double originalDemand = -supply[i];
            double served = 0.0;
            for (const auto& fEdge : network.adjacency[i]) {
                if (fEdge.originalEdgeId >= 0) {
                    if (fEdge.to == i && fEdge.flow > 0) {
                        served += fEdge.flow;
                    }
                }
            }
            if (served < originalDemand - 1e-6) {
                result.unmetDemand.push_back(i);
            }
        }
    }

    result.totalFlow = totalFlow;
    result.totalCost = totalCost;
    result.success = true;
    return result;
}

// ==================== Build Flow Network ====================

MinCostMaxFlow::FlowNetwork MinCostMaxFlow::buildNetwork(
    const Grid& grid,
    const std::vector<double>& supply
) {
    FlowNetwork network;
    int n = grid.nodeCount();

    // Flow network: original nodes + super-source + super-sink
    network.numGridNodes = n;
    network.superSource = n;
    network.superSink = n + 1;
    network.numNodes = n + 2;
    network.adjacency.resize(network.numNodes);

    // Add edges for each original grid edge (bidirectional in flow network)
    for (const auto& edge : grid.getEdges()) {
        if (edge.tripped) continue;

        // Compute cost based on edge resistance and connected loads
        double baseCost = edge.resistance;

        // Forward edge (from -> to)
        int fwdRevIdx = static_cast<int>(network.adjacency[edge.to].size());
        int revRevIdx = static_cast<int>(network.adjacency[edge.from].size());

        network.adjacency[edge.from].emplace_back(
            edge.from, edge.to, edge.capacityMW, baseCost,
            fwdRevIdx, edge.id
        );

        // Reverse edge (to -> from) with zero capacity initially and negative cost
        network.adjacency[edge.to].emplace_back(
            edge.to, edge.from, 0.0, -baseCost,
            revRevIdx, edge.id
        );

        // Update the reverse indices
        network.adjacency[edge.from].back().reverseEdgeIndex =
            static_cast<int>(network.adjacency[edge.to].size()) - 1;
        network.adjacency[edge.to].back().reverseEdgeIndex =
            static_cast<int>(network.adjacency[edge.from].size()) - 1;
    }

    // Connect super-source to generators
    for (int i = 0; i < n; ++i) {
        if (supply[i] > 1e-6) {
            int revIdx = static_cast<int>(network.adjacency[i].size());
            network.adjacency[network.superSource].emplace_back(
                network.superSource, i, supply[i], 0.0,
                revIdx, -1
            );
            network.adjacency[i].emplace_back(
                i, network.superSource, 0.0, 0.0,
                static_cast<int>(network.adjacency[network.superSource].size()) - 1, -1
            );
        }
    }

    // Connect loads to super-sink
    for (int i = 0; i < n; ++i) {
        if (supply[i] < -1e-6) {
            double demand = -supply[i];
            int revIdx = static_cast<int>(network.adjacency[network.superSink].size());
            network.adjacency[i].emplace_back(
                i, network.superSink, demand, 0.0,
                revIdx, -1
            );
            network.adjacency[network.superSink].emplace_back(
                network.superSink, i, 0.0, 0.0,
                static_cast<int>(network.adjacency[i].size()) - 1, -1
            );
        }
    }

    return network;
}

// ==================== Find Augmenting Path (SPFA) ====================

bool MinCostMaxFlow::findAugmentingPath(
    const FlowNetwork& network,
    int source,
    int sink,
    std::vector<int>& predecessorNode,
    std::vector<int>& predecessorEdge
) {
    int n = network.numNodes;
    std::vector<double> distance(n, std::numeric_limits<double>::infinity());
    std::vector<bool> inQueue(n, false);
    std::vector<int> relaxCount(n, 0);
    std::queue<int> q;

    distance[source] = 0.0;
    q.push(source);
    inQueue[source] = true;
    relaxCount[source] = 1;

    while (!q.empty()) {
        int u = q.front();
        q.pop();
        inQueue[u] = false;

        for (size_t i = 0; i < network.adjacency[u].size(); ++i) {
            const MCMFEdge& edge = network.adjacency[u][i];
            if (edge.capacity < 1e-9) continue; // no remaining capacity

            int v = edge.to;
            double newDist = distance[u] + edge.cost;

            if (newDist < distance[v] - 1e-9) {
                distance[v] = newDist;
                predecessorNode[v] = u;
                predecessorEdge[v] = static_cast<int>(i);

                if (!inQueue[v]) {
                    q.push(v);
                    inQueue[v] = true;
                    relaxCount[v]++;
                    if (relaxCount[v] > n) {
                        return false; // negative cycle
                    }
                }
            }
        }
    }

    return distance[sink] < std::numeric_limits<double>::infinity() / 2.0;
}

// ==================== Augment Flow ====================

double MinCostMaxFlow::augment(
    FlowNetwork& network,
    int source,
    int sink,
    const std::vector<int>& predecessorNode,
    const std::vector<int>& predecessorEdge
) {
    // Find bottleneck capacity along the path
    double bottleneck = std::numeric_limits<double>::infinity();
    int current = sink;

    while (current != source) {
        int prev = predecessorNode[current];
        int edgeIdx = predecessorEdge[current];
        if (prev < 0 || edgeIdx < 0) return 0.0;

        const MCMFEdge& edge = network.adjacency[prev][edgeIdx];
        bottleneck = std::min(bottleneck, edge.capacity);
        current = prev;
    }

    if (bottleneck < 1e-9) return 0.0;

    // Push flow along the path
    current = sink;
    while (current != source) {
        int prev = predecessorNode[current];
        int edgeIdx = predecessorEdge[current];

        MCMFEdge& forward = network.adjacency[prev][edgeIdx];
        MCMFEdge& reverse = network.adjacency[current][forward.reverseEdgeIndex];

        forward.flow += bottleneck;
        forward.capacity -= bottleneck;
        reverse.capacity += bottleneck;
        reverse.flow -= bottleneck;

        current = prev;
    }

    return bottleneck;
}

} // namespace gridpulse