"""
Engine Service — thin wrapper around the C++ engine via pybind11.
Handles all communication with the compiled gridpulse_engine_py module.
"""

import sys
import os

# The .pyd and DLLs are in the backend root folder
_BACKEND_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
os.add_dll_directory(_BACKEND_ROOT)
sys.path.insert(0, _BACKEND_ROOT)

import gridpulse_engine_py as _engine


def generate_grid(config_dict: dict):
    """
    Generate a synthetic grid from a configuration dictionary.
    
    Args:
        config_dict: {
            "totalNodes": int,
            "generatorRatio": float,
            "substationRatio": float,
            "redundancyFactor": int,
            "canvasWidth": float,
            "canvasHeight": float,
            "seed": int
        }
    
    Returns:
        dict with grid data serialized for frontend consumption
    """
    config = _engine.GeneratorConfig()
    config.totalNodes = config_dict.get("totalNodes", 40)
    config.generatorRatio = config_dict.get("generatorRatio", 0.15)
    config.substationRatio = config_dict.get("substationRatio", 0.25)
    config.redundancyFactor = config_dict.get("redundancyFactor", 5)
    config.canvasWidth = config_dict.get("canvasWidth", 800.0)
    config.canvasHeight = config_dict.get("canvasHeight", 600.0)
    config.seed = config_dict.get("seed", 42)
    
    grid = _engine.generateGrid(config)
    return _serialize_grid(grid)


def run_cascade(grid_config_dict: dict, trigger_dict: dict):
    """
    Run a cascade simulation on a freshly generated grid.
    
    Args:
        grid_config_dict: same as generate_grid
        trigger_dict: {
            "type": "CUT_EDGE" | "FAIL_NODE" | "SPIKE_DEMAND",
            "targetId": int,
            "value": float (optional, for SPIKE_DEMAND)
        }
    
    Returns:
        dict with cascade results including frames array
    """
    config = _engine.GeneratorConfig()
    config.totalNodes = grid_config_dict.get("totalNodes", 30)
    config.generatorRatio = grid_config_dict.get("generatorRatio", 0.15)
    config.substationRatio = grid_config_dict.get("substationRatio", 0.25)
    config.redundancyFactor = grid_config_dict.get("redundancyFactor", 5)
    config.seed = grid_config_dict.get("seed", 42)
    
    grid = _engine.generateGrid(config)
    
    trigger = _engine.TriggerEvent()
    trigger_type = trigger_dict.get("type", "CUT_EDGE")
    if trigger_type == "CUT_EDGE":
        trigger.type = _engine.TriggerType.CUT_EDGE
    elif trigger_type == "FAIL_NODE":
        trigger.type = _engine.TriggerType.FAIL_NODE
    elif trigger_type == "SPIKE_DEMAND":
        trigger.type = _engine.TriggerType.SPIKE_DEMAND
    trigger.targetId = trigger_dict.get("targetId", 0)
    trigger.value = trigger_dict.get("value", 0.0)
    
    result = _engine.runCascade(grid, trigger)
    
    # Add grid data to each frame for frontend rendering
    grid_data = _serialize_grid(grid)
    for frame in result.get("frames", []):
        frame["nodes"] = grid_data["nodes"]
        frame["edges"] = grid_data["edges"]
    
    return dict(result)

def init_storm_simulator(grid_config_dict: dict, storm_config_dict: dict):
    """Initialize a storm simulator with a map grid."""
    # Generate the map grid
    map_config = _engine.MapConfig()
    map_config.mapWidth = grid_config_dict.get("mapWidth", 1200)
    map_config.mapHeight = grid_config_dict.get("mapHeight", 800)
    map_config.numPowerPlants = grid_config_dict.get("numPowerPlants", 3)
    map_config.numSubstations = grid_config_dict.get("numSubstations", 5)
    map_config.numHospitals = grid_config_dict.get("numHospitals", 2)
    map_config.numCommercial = grid_config_dict.get("numCommercial", 8)
    map_config.numResidential = grid_config_dict.get("numResidential", 20)
    map_config.redundancyFactor = grid_config_dict.get("redundancyFactor", 5)
    map_config.seed = grid_config_dict.get("seed", 42)

    grid = _engine.generateMapGrid(map_config)

    # Configure storm
    storm_config = _engine.StormConfig()
    storm_config.duration = storm_config_dict.get("duration", 120.0)
    storm_config.intensity = storm_config_dict.get("intensity", 0.6)
    storm_config.numInitialFailures = storm_config_dict.get("numInitialFailures", 3)
    storm_config.seed = storm_config_dict.get("seed", grid_config_dict.get("seed", 42))
    storm_config.playerBudget = storm_config_dict.get("playerBudget", 100.0)

    # Create simulator
    sim = _engine.StormSimulator(storm_config)
    sim.initialize(grid)

    # Serialize grid for frontend
    grid_data = _serialize_grid(sim.getGrid())
    return grid_data, sim

def compute_flow(grid_config_dict: dict):
    """Compute optimal power flow for a grid."""
    config = _engine.GeneratorConfig()
    config.totalNodes = grid_config_dict.get("totalNodes", 30)
    config.seed = grid_config_dict.get("seed", 42)
    config.generatorRatio = grid_config_dict.get("generatorRatio", 0.15)
    config.redundancyFactor = grid_config_dict.get("redundancyFactor", 5)
    
    grid = _engine.generateGrid(config)
    result = _engine.computeFlow(grid)
    return dict(result)

def generate_map_grid(config_dict: dict):
    """Generate a city-style map grid."""
    config = _engine.MapConfig()
    config.mapWidth = config_dict.get("mapWidth", 1200)
    config.mapHeight = config_dict.get("mapHeight", 800)
    config.numPowerPlants = config_dict.get("numPowerPlants", 3)
    config.numSubstations = config_dict.get("numSubstations", 5)
    config.numHospitals = config_dict.get("numHospitals", 2)
    config.numCommercial = config_dict.get("numCommercial", 8)
    config.numResidential = config_dict.get("numResidential", 20)
    config.redundancyFactor = config_dict.get("redundancyFactor", 5)
    config.seed = config_dict.get("seed", 42)

    grid = _engine.generateMapGrid(config)
    return _serialize_grid(grid)

def analyze_grid(grid_config_dict: dict):
    """Run vulnerability analysis on a grid."""
    config = _engine.GeneratorConfig()
    config.totalNodes = grid_config_dict.get("totalNodes", 30)
    config.seed = grid_config_dict.get("seed", 42)
    config.generatorRatio = grid_config_dict.get("generatorRatio", 0.15)
    config.redundancyFactor = grid_config_dict.get("redundancyFactor", 5)
    
    grid = _engine.generateGrid(config)
    betweenness = _engine.computeBetweenness(grid)
    articulation = _engine.findArticulationPoints(grid)
    
    return {
        "betweenness": dict(betweenness),
        "articulationPoints": dict(articulation)
    }

def generate_map_grid(config_dict: dict):
    """Generate a city-style map grid with realistic layout."""
    config = _engine.MapConfig()
    config.mapWidth = config_dict.get("mapWidth", 1200)
    config.mapHeight = config_dict.get("mapHeight", 800)
    config.numPowerPlants = config_dict.get("numPowerPlants", 3)
    config.numSubstations = config_dict.get("numSubstations", 5)
    config.numHospitals = config_dict.get("numHospitals", 2)
    config.numCommercial = config_dict.get("numCommercial", 8)
    config.numResidential = config_dict.get("numResidential", 20)
    config.redundancyFactor = config_dict.get("redundancyFactor", 5)
    config.seed = config_dict.get("seed", 42)
    
    grid = _engine.generateMapGrid(config)
    return _serialize_grid(grid)

def _serialize_grid(grid) -> dict:
    """Convert a C++ Grid object to a frontend-friendly dict."""
    nodes = []
    for i in range(grid.nodeCount()):
        node = grid.getNode(i)
        nodes.append({
            "id": node.id,
            "type": node.typeString(),
            "capacityMW": node.capacityMW,
            "currentLoadMW": node.currentLoadMW,
            "tier": node.tierString(),
            "x": node.x,
            "y": node.y
        })
    
    edges = []
    for i in range(grid.edgeCount()):
        edge = grid.getEdge(i)
    edges.append({
    "id": edge.id,
    "from": getattr(edge, "from"),
    "to": getattr(edge, "to"),
    "capacityMW": edge.capacityMW,
    "currentFlowMW": edge.currentFlowMW,
    "resistance": edge.resistance,
    "tripped": edge.tripped
})
    
    return {"nodes": nodes, "edges": edges}