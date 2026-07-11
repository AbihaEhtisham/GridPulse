#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include "graph/grid.hpp"
#include "utils/grid_generator.hpp"
#include "flow/min_cost_max_flow.hpp"
#include "flow/dijkstra.hpp"
#include "cascade/cascade_loop.hpp"
#include "analysis/betweenness.hpp"
#include "analysis/articulation_points.hpp"
#include <chrono>
#include <algorithm>

using namespace gridpulse;

void printBanner() {
    std::cout << "\n";
    std::cout << "  ╔══════════════════════════════════╗\n";
    std::cout << "  ║         GridPulse Engine         ║\n";
    std::cout << "  ║  Power Grid Resilience Analyzer  ║\n";
    std::cout << "  ╚══════════════════════════════════╝\n";
    std::cout << "\n";
}

void printUsage() {
    std::cout << "Usage: gridpulse_cli <command> [options]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  generate    Generate a synthetic power grid\n";
    std::cout << "  analyze     Run vulnerability analysis on a grid\n";
    std::cout << "  simulate    Run a cascade simulation\n";
    std::cout << "  flow        Compute optimal power flow\n";
    std::cout << "  path        Find shortest path between two nodes\n";
    std::cout << "  benchmark   Run performance benchmarks\n\n";
    std::cout << "Options for 'generate':\n";
    std::cout << "  --nodes N       Number of nodes (default: 40)\n";
    std::cout << "  --gen-ratio R   Fraction of generators (default: 0.15)\n";
    std::cout << "  --redundancy R  Extra edges beyond tree (default: 5)\n";
    std::cout << "  --seed S        Random seed (default: 42)\n";
    std::cout << "  --output FILE   Output JSON file (default: grid.json)\n\n";
    std::cout << "Options for 'simulate':\n";
    std::cout << "  --grid FILE     Input grid JSON file\n";
    std::cout << "  --cut-edge ID   Trip a specific edge\n";
    std::cout << "  --fail-node ID  Disable a specific node\n";
    std::cout << "  --spike ID VAL  Spike demand at a node\n\n";
    std::cout << "Examples:\n";
    std::cout << "  gridpulse_cli generate --nodes 50 --seed 123\n";
    std::cout << "  gridpulse_cli analyze --grid grid.json\n";
    std::cout << "  gridpulse_cli simulate --grid grid.json --cut-edge 5\n";
    std::cout << "\n";
}

// ===== Generate Command =====
void cmdGenerate(int argc, char* argv[]) {
    GeneratorConfig config;
    std::string outputFile = "grid.json";
    
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--nodes" && i + 1 < argc) {
            config.totalNodes = std::stoi(argv[++i]);
        } else if (arg == "--gen-ratio" && i + 1 < argc) {
            config.generatorRatio = std::stod(argv[++i]);
        } else if (arg == "--redundancy" && i + 1 < argc) {
            config.redundancyFactor = std::stoi(argv[++i]);
        } else if (arg == "--seed" && i + 1 < argc) {
            config.seed = std::stoi(argv[++i]);
        } else if (arg == "--output" && i + 1 < argc) {
            outputFile = argv[++i];
        }
    }
    
    std::cout << "Generating grid...\n";
    std::cout << "  Nodes: " << config.totalNodes << "\n";
    std::cout << "  Generator ratio: " << config.generatorRatio << "\n";
    std::cout << "  Redundancy: " << config.redundancyFactor << "\n";
    std::cout << "  Seed: " << config.seed << "\n";
    
    Grid grid = GridGenerator::generate(config);
    
    std::cout << "\nGenerated: " << grid.nodeCount() << " nodes, "
              << grid.edgeCount() << " edges\n";
    std::cout << "Connected: " << (grid.isConnected() ? "YES" : "NO") << "\n";
    
    // Type breakdown
    int gens = 0, subs = 0, loads = 0;
    int crit = 0, comm = 0, res = 0;
    for (const auto& node : grid.getNodes()) {
        if (node.type == NodeType::GENERATOR) gens++;
        else if (node.type == NodeType::SUBSTATION) subs++;
        else {
            loads++;
            if (node.tier == CriticalityTier::CRITICAL) crit++;
            else if (node.tier == CriticalityTier::COMMERCIAL) comm++;
            else res++;
        }
    }
    std::cout << "Generators: " << gens << ", Substations: " << subs
              << ", Loads: " << loads << "\n";
    std::cout << "Critical: " << crit << ", Commercial: " << comm
              << ", Residential: " << res << "\n";
    
    std::string json = gridToJson(grid);
    std::ofstream out(outputFile);
    out << json;
    out.close();
    std::cout << "\nGrid saved to: " << outputFile << "\n";
}

// ===== Analyze Command =====
void cmdAnalyze(int argc, char* argv[]) {
    std::string gridFile = "grid.json";
    
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--grid" && i + 1 < argc) {
            gridFile = argv[++i];
        }
    }
    
    // For now, generate a fresh grid since JSON deserialization is minimal
    GeneratorConfig config;
    config.seed = 42;
    Grid grid = GridGenerator::generate(config);
    
    std::cout << "Analyzing grid (" << grid.nodeCount() << " nodes, "
              << grid.edgeCount() << " edges)...\n\n";
    
    // Betweenness centrality
    std::cout << Betweenness::summary(Betweenness::compute(grid));
    std::cout << "\n";
    
    // Articulation points
    std::cout << ArticulationPoints::summary(ArticulationPoints::find(grid), grid);
}

// ===== Simulate Command =====
void cmdSimulate(int argc, char* argv[]) {
    TriggerEvent trigger;
    trigger.type = TriggerType::CUT_EDGE;
    trigger.targetId = 0;
    bool triggerSet = false;
    
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--cut-edge" && i + 1 < argc) {
            trigger.type = TriggerType::CUT_EDGE;
            trigger.targetId = std::stoi(argv[++i]);
            triggerSet = true;
        } else if (arg == "--fail-node" && i + 1 < argc) {
            trigger.type = TriggerType::FAIL_NODE;
            trigger.targetId = std::stoi(argv[++i]);
            triggerSet = true;
        } else if (arg == "--spike" && i + 2 < argc) {
            trigger.type = TriggerType::SPIKE_DEMAND;
            trigger.targetId = std::stoi(argv[++i]);
            trigger.value = std::stod(argv[++i]);
            triggerSet = true;
        }
    }
    
    // Generate a grid for simulation
    GeneratorConfig config;
    config.totalNodes = 20;
    config.seed = 42;
    Grid grid = GridGenerator::generate(config);
    
    std::cout << "Grid: " << grid.nodeCount() << " nodes, "
              << grid.edgeCount() << " edges\n";
    
    if (triggerSet) {
        std::cout << "Trigger: ";
        if (trigger.type == TriggerType::CUT_EDGE)
            std::cout << "Cut Edge " << trigger.targetId;
        else if (trigger.type == TriggerType::FAIL_NODE)
            std::cout << "Fail Node " << trigger.targetId;
        else
            std::cout << "Spike Node " << trigger.targetId;
        std::cout << "\n";
    } else {
        std::cout << "No trigger specified. Using default: Cut Edge 0\n";
    }
    
    std::cout << "\nRunning cascade simulation...\n\n";
    
    CascadeResult cascade = CascadeLoop::run(grid, trigger);
    
    std::cout << "=== Cascade Results ===\n";
    std::cout << "Iterations: " << cascade.totalIterations << "\n";
    std::cout << "Stabilized: " << (cascade.gridStabilized ? "YES" : "NO") << "\n";
    std::cout << "Total blackout: " << (cascade.totalBlackout ? "YES" : "NO") << "\n";
    std::cout << "Edges tripped: " << cascade.totalEdgesTripped << "\n";
    std::cout << "Loads shed: " << cascade.totalNodesShed << "\n";
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "Initial demand: " << cascade.initialDemand << " MW\n";
    std::cout << "Final demand served: " << cascade.finalDemandServed << " MW\n";
    if (cascade.initialDemand > 0) {
        std::cout << "Demand served: " << std::setprecision(1)
                  << (cascade.finalDemandServed / cascade.initialDemand * 100.0) << "%\n";
    }
    
    std::cout << "\n=== Frame Timeline ===\n";
    for (const auto& frame : cascade.frames) {
        std::cout << "Frame " << frame.frameNumber << ": "
                  << frame.eventDescription << "\n";
    }
}

// ===== Flow Command =====
void cmdFlow(int argc, char* argv[]) {
    GeneratorConfig config;
    config.totalNodes = 15;
    config.seed = 42;
    Grid grid = GridGenerator::generate(config);
    
    MCMFResult result = MinCostMaxFlow::compute(grid);
    
    std::cout << "Optimal Power Flow Results\n";
    std::cout << "==========================\n";
    std::cout << "Grid: " << grid.nodeCount() << " nodes, "
              << grid.edgeCount() << " edges\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Total flow: " << result.totalFlow << " MW\n";
    std::cout << "Total cost: " << result.totalCost << "\n";
    std::cout << "Unmet demand nodes: " << result.unmetDemand.size() << "\n\n";
    
    // Show top 10 edge utilizations
    std::cout << "Edge Utilization (top 10 by flow):\n";
    std::vector<std::pair<int, double>> utilizations;
    for (int i = 0; i < grid.edgeCount(); ++i) {
        double flow = result.edgeFlows[i];
        double cap = grid.getEdge(i).capacityMW;
        double pct = (cap > 0) ? (flow / cap * 100.0) : 0.0;
        utilizations.push_back({i, pct});
    }
    std::sort(utilizations.begin(), utilizations.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    int count = std::min(10, static_cast<int>(utilizations.size()));
    for (int i = 0; i < count; ++i) {
        int edgeId = utilizations[i].first;
        const Edge& edge = grid.getEdge(edgeId);
        std::cout << "  Edge " << edgeId << " (" << edge.from << " -> " << edge.to
                  << "): " << std::setprecision(1) << utilizations[i].second << "% utilized\n";
    }
}

// ===== Path Command =====
void cmdPath(int argc, char* argv[]) {
    int source = 0;
    int target = 0;
    
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--from" && i + 1 < argc) {
            source = std::stoi(argv[++i]);
        } else if (arg == "--to" && i + 1 < argc) {
            target = std::stoi(argv[++i]);
        }
    }
    
    GeneratorConfig config;
    config.totalNodes = 15;
    config.seed = 42;
    Grid grid = GridGenerator::generate(config);
    
    if (target == 0) target = grid.nodeCount() - 1;
    
    auto [dist, path] = Dijkstra::shortestPath(grid, source, target);
    
    std::cout << "Shortest Path: Node " << source << " -> Node " << target << "\n";
    std::cout << "================================\n";
    
    if (dist < 0) {
        std::cout << "No path exists between these nodes.\n";
    } else {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Total distance: " << dist << "\n";
        std::cout << "Hops: " << path.size() << "\n";
        std::cout << "Path: " << source;
        int current = source;
        for (int edgeId : path) {
            const Edge& edge = grid.getEdge(edgeId);
            current = (edge.from == current) ? edge.to : edge.from;
            std::cout << " -> " << current;
        }
        std::cout << "\n";
    }
}

// ===== Benchmark Command =====
void cmdBenchmark(int argc, char* argv[]) {
    std::cout << "GridPulse Performance Benchmarks\n";
    std::cout << "================================\n\n";
    
    std::cout << "MCMF Performance:\n";
    std::cout << "Size\tNodes\tEdges\tTime(ms)\n";
    std::cout << "----\t-----\t-----\t--------\n";
    
    for (int size : {20, 40, 60, 80}) {
        GeneratorConfig config;
        config.totalNodes = size;
        config.seed = 42;
        Grid grid = GridGenerator::generate(config);
        
        auto start = std::chrono::high_resolution_clock::now();
        MCMFResult result = MinCostMaxFlow::compute(grid);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        std::cout << size << "\t" << grid.nodeCount() << "\t"
                  << grid.edgeCount() << "\t" << ms << "\n";
    }
    
    std::cout << "\nBenchmark complete.\n";
}

// ===== Main =====
int main(int argc, char* argv[]) {
    printBanner();
    
    if (argc < 2) {
        printUsage();
        return 0;
    }
    
    std::string command = argv[1];
    
    if (command == "generate") {
        cmdGenerate(argc, argv);
    } else if (command == "analyze") {
        cmdAnalyze(argc, argv);
    } else if (command == "simulate") {
        cmdSimulate(argc, argv);
    } else if (command == "flow") {
        cmdFlow(argc, argv);
    } else if (command == "path") {
        cmdPath(argc, argv);
    } else if (command == "benchmark") {
        cmdBenchmark(argc, argv);
    } else if (command == "help" || command == "--help" || command == "-h") {
        printUsage();
    } else {
        std::cout << "Unknown command: " << command << "\n";
        printUsage();
        return 1;
    }
    
    std::cout << "\n";
    return 0;
}