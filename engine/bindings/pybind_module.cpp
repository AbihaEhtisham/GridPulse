#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include "graph/grid.hpp"
#include "graph/node.hpp"
#include "graph/edge.hpp"
#include "utils/grid_generator.hpp"
#include "utils/map_generator.hpp"
#include "flow/min_cost_max_flow.hpp"
#include "flow/dijkstra.hpp"
#include "cascade/cascade_loop.hpp"
#include "cascade/frame.hpp"
#include "analysis/betweenness.hpp"
#include "analysis/articulation_points.hpp"

namespace py = pybind11;
using namespace gridpulse;

// Helper to convert MCMFResult to Python dict
py::dict mcmfResultToDict(const MCMFResult& result) {
    py::dict d;
    d["totalFlow"] = result.totalFlow;
    d["totalCost"] = result.totalCost;
    d["nodeSupply"] = result.nodeSupply;
    d["edgeFlows"] = result.edgeFlows;
    d["unmetDemand"] = result.unmetDemand;
    d["success"] = result.success;
    return d;
}

// Helper to convert Frame to Python dict
py::dict frameToDict(const Frame& frame) {
    py::dict d;
    d["frameNumber"] = frame.frameNumber;
    d["nodeLoads"] = frame.nodeLoads;
    d["edgeFlows"] = frame.edgeFlows;
    d["edgeTripped"] = frame.edgeTripped;
    d["islandIds"] = frame.islandIds;
    d["nodeShed"] = frame.nodeShed;
    d["eventDescription"] = frame.eventDescription;
    return d;
}

// Helper to convert CascadeResult to Python dict
py::dict cascadeResultToDict(const CascadeResult& result) {
    py::dict d;
    py::list frames;
    for (const auto& frame : result.frames) {
        frames.append(frameToDict(frame));
    }
    d["frames"] = frames;
    d["totalIterations"] = result.totalIterations;
    d["gridStabilized"] = result.gridStabilized;
    d["totalBlackout"] = result.totalBlackout;
    d["finalDemandServed"] = result.finalDemandServed;
    d["initialDemand"] = result.initialDemand;
    d["totalEdgesTripped"] = result.totalEdgesTripped;
    d["totalNodesShed"] = result.totalNodesShed;
    return d;
}

// Helper to convert BetweennessResult to Python dict
py::dict betweennessResultToDict(const BetweennessResult& result) {
    py::dict d;
    d["edgeScores"] = result.edgeScores;
    d["nodeScores"] = result.nodeScores;
    d["maxEdgeScore"] = result.maxEdgeScore;
    d["maxNodeScore"] = result.maxNodeScore;
    return d;
}

// Helper to convert ArticulationResult to Python dict
py::dict articulationResultToDict(const ArticulationResult& result) {
    py::dict d;
    d["articulationPoints"] = result.articulationPoints;
    d["totalNodes"] = result.totalNodes;
    d["articulationCount"] = result.articulationCount;
    return d;
}

PYBIND11_MODULE(gridpulse_engine_py, m) {
    m.doc() = "GridPulse Engine - Power Grid Resilience Analysis";

    // ===== Enums =====
    py::enum_<NodeType>(m, "NodeType")
        .value("GENERATOR", NodeType::GENERATOR)
        .value("SUBSTATION", NodeType::SUBSTATION)
        .value("LOAD", NodeType::LOAD)
        .export_values();

    py::enum_<CriticalityTier>(m, "CriticalityTier")
        .value("CRITICAL", CriticalityTier::CRITICAL)
        .value("COMMERCIAL", CriticalityTier::COMMERCIAL)
        .value("RESIDENTIAL", CriticalityTier::RESIDENTIAL)
        .export_values();

    py::enum_<TriggerType>(m, "TriggerType")
        .value("CUT_EDGE", TriggerType::CUT_EDGE)
        .value("FAIL_NODE", TriggerType::FAIL_NODE)
        .value("SPIKE_DEMAND", TriggerType::SPIKE_DEMAND)
        .export_values();

    // ===== Node =====
    py::class_<Node>(m, "Node")
        .def(py::init<>())
        .def_readonly("id", &Node::id)
        .def_readonly("type", &Node::type)
        .def_readonly("capacityMW", &Node::capacityMW)
        .def_readonly("currentLoadMW", &Node::currentLoadMW)
        .def_readonly("tier", &Node::tier)
        .def_readonly("x", &Node::x)
        .def_readonly("y", &Node::y)
        .def("typeString", &Node::typeString)
        .def("tierString", &Node::tierString);

    // ===== Edge =====
    py::class_<Edge>(m, "Edge")
        .def(py::init<>())
        .def_readonly("id", &Edge::id)
        .def_readonly("from", &Edge::from)
        .def_readonly("to", &Edge::to)
        .def_readonly("capacityMW", &Edge::capacityMW)
        .def_readonly("currentFlowMW", &Edge::currentFlowMW)
        .def_readonly("resistance", &Edge::resistance)
        .def_readonly("tripped", &Edge::tripped)
        .def("isOverloaded", &Edge::isOverloaded, py::arg("tolerance") = 0.01);

    // ===== Grid =====
    py::class_<Grid>(m, "Grid")
        .def(py::init<>())
        .def("addNode", &Grid::addNode,
             py::arg("type"), py::arg("capacityMW"), py::arg("tier"),
             py::arg("x"), py::arg("y"))
        .def("addEdge", &Grid::addEdge,
             py::arg("from"), py::arg("to"), py::arg("capacityMW"), py::arg("resistance"))
        .def("getNode", py::overload_cast<int>(&Grid::getNode), py::return_value_policy::reference)
        .def("getEdge", py::overload_cast<int>(&Grid::getEdge), py::return_value_policy::reference)
        .def("nodeCount", &Grid::nodeCount)
        .def("edgeCount", &Grid::edgeCount)
        .def("getNeighbors", &Grid::getNeighbors)
        .def("isConnected", &Grid::isConnected)
        .def("removeNode", &Grid::removeNode)
        .def("removeEdge", &Grid::removeEdge)
        .def("resetFlows", &Grid::resetFlows)
        .def("toJson", &gridToJson)
        .def_static("fromJson", &gridFromJson);

    // ===== Generator Config =====
    py::class_<GeneratorConfig>(m, "GeneratorConfig")
        .def(py::init<>())
        .def_readwrite("totalNodes", &GeneratorConfig::totalNodes)
        .def_readwrite("generatorRatio", &GeneratorConfig::generatorRatio)
        .def_readwrite("substationRatio", &GeneratorConfig::substationRatio)
        .def_readwrite("redundancyFactor", &GeneratorConfig::redundancyFactor)
        .def_readwrite("canvasWidth", &GeneratorConfig::canvasWidth)
        .def_readwrite("canvasHeight", &GeneratorConfig::canvasHeight)
        .def_readwrite("seed", &GeneratorConfig::seed);

    // ===== Map Config =====
    py::class_<MapConfig>(m, "MapConfig")
        .def(py::init<>())
        .def_readwrite("mapWidth", &MapConfig::mapWidth)
        .def_readwrite("mapHeight", &MapConfig::mapHeight)
        .def_readwrite("numPowerPlants", &MapConfig::numPowerPlants)
        .def_readwrite("numSubstations", &MapConfig::numSubstations)
        .def_readwrite("numHospitals", &MapConfig::numHospitals)
        .def_readwrite("numCommercial", &MapConfig::numCommercial)
        .def_readwrite("numResidential", &MapConfig::numResidential)
        .def_readwrite("redundancyFactor", &MapConfig::redundancyFactor)
        .def_readwrite("seed", &MapConfig::seed);

    // ===== Grid Generator =====
    m.def("generateGrid", [](const GeneratorConfig& config) {
        return GridGenerator::generate(config);
    }, py::arg("config"), "Generate a synthetic power grid");

    // ===== Map Grid Generator =====
    m.def("generateMapGrid", [](const MapConfig& config) {
        return MapGridGenerator::generate(config);
    }, py::arg("config"), "Generate a city-style map grid with realistic layout");

    // ===== Trigger Event =====
    py::class_<TriggerEvent>(m, "TriggerEvent")
        .def(py::init<>())
        .def_readwrite("type", &TriggerEvent::type)
        .def_readwrite("targetId", &TriggerEvent::targetId)
        .def_readwrite("value", &TriggerEvent::value);

    // ===== MCMF =====
    m.def("computeFlow", [](Grid& grid) {
        MCMFResult result = MinCostMaxFlow::compute(grid);
        return mcmfResultToDict(result);
    }, py::arg("grid"), "Compute optimal power flow");

    // ===== Dijkstra =====
    m.def("shortestPath", [](Grid& grid, int source, int target) {
        auto [dist, path] = Dijkstra::shortestPath(grid, source, target);
        return py::make_tuple(dist, path);
    }, py::arg("grid"), py::arg("source"), py::arg("target"),
       "Find shortest path between two nodes");

    // ===== Cascade Loop =====
    m.def("runCascade", [](Grid& grid, TriggerEvent& trigger) {
        CascadeResult result = CascadeLoop::run(grid, trigger);
        return cascadeResultToDict(result);
    }, py::arg("grid"), py::arg("trigger"),
       "Run a cascade simulation");

    // ===== Analysis =====
    m.def("computeBetweenness", [](Grid& grid) {
        BetweennessResult result = Betweenness::compute(grid);
        return betweennessResultToDict(result);
    }, py::arg("grid"), "Compute betweenness centrality");

    m.def("findArticulationPoints", [](Grid& grid) {
        ArticulationResult result = ArticulationPoints::find(grid);
        return articulationResultToDict(result);
    }, py::arg("grid"), "Find articulation points");
}
