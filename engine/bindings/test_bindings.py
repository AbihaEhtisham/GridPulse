"""
Test script for GridPulse Python bindings.
Run from: python engine/bindings/test_bindings.py
"""

import sys
import os

# Add the build directory to both sys.path and DLL search path
BUILD_DIR = os.path.join(os.path.dirname(__file__), '..', 'build')
sys.path.insert(0, BUILD_DIR)
os.add_dll_directory(BUILD_DIR)

try:
    import gridpulse_engine_py as gp
    print("✅ Successfully imported gridpulse_engine_py")
except ImportError as e:
    print(f"❌ Import failed: {e}")
    sys.exit(1)


def test_grid_generation():
    print("\n=== Test 1: Grid Generation ===")
    config = gp.GeneratorConfig()
    config.totalNodes = 20
    config.generatorRatio = 0.2
    config.redundancyFactor = 3
    config.seed = 42

    grid = gp.generateGrid(config)
    print(f"  Nodes: {grid.nodeCount()}")
    print(f"  Edges: {grid.edgeCount()}")
    print(f"  Connected: {grid.isConnected()}")

    # Check node types
    gen_count = 0
    load_count = 0
    for i in range(grid.nodeCount()):
        node = grid.getNode(i)
        if node.type == gp.NodeType.GENERATOR:
            gen_count += 1
        elif node.type == gp.NodeType.LOAD:
            load_count += 1

    print(f"  Generators: {gen_count}")
    print(f"  Loads: {load_count}")

    assert grid.nodeCount() == 20, "Node count mismatch"
    assert grid.isConnected(), "Grid should be connected"
    print("  ✅ Passed")


def test_mcmf():
    print("\n=== Test 2: Min-Cost Max-Flow ===")
    config = gp.GeneratorConfig()
    config.totalNodes = 15
    config.seed = 42
    grid = gp.generateGrid(config)

    result = gp.computeFlow(grid)
    print(f"  Total flow: {result['totalFlow']:.2f} MW")
    print(f"  Total cost: {result['totalCost']:.2f}")
    print(f"  Unmet demand nodes: {len(result['unmetDemand'])}")
    print(f"  Edge flows computed: {len(result['edgeFlows'])}")
    assert result['success'], "MCMF should succeed"
    print("  ✅ Passed")


def test_dijkstra():
    print("\n=== Test 3: Dijkstra Shortest Path ===")
    config = gp.GeneratorConfig()
    config.totalNodes = 10
    config.seed = 99
    grid = gp.generateGrid(config)

    source = 0
    target = min(5, grid.nodeCount() - 1)
    dist, path = gp.shortestPath(grid, source, target)
    print(f"  Path from {source} to {target}:")
    print(f"    Distance: {dist:.2f}")
    print(f"    Hops: {len(path)}")
    print(f"    Edges: {path}")
    print("  ✅ Passed")


def test_cascade():
    print("\n=== Test 4: Cascade Simulation ===")
    config = gp.GeneratorConfig()
    config.totalNodes = 15
    config.seed = 42
    grid = gp.generateGrid(config)

    trigger = gp.TriggerEvent()
    trigger.type = gp.TriggerType.CUT_EDGE
    trigger.targetId = 0

    result = gp.runCascade(grid, trigger)
    print(f"  Iterations: {result['totalIterations']}")
    print(f"  Stabilized: {result['gridStabilized']}")
    print(f"  Total blackout: {result['totalBlackout']}")
    print(f"  Edges tripped: {result['totalEdgesTripped']}")
    print(f"  Loads shed: {result['totalNodesShed']}")
    print(f"  Frames recorded: {len(result['frames'])}")

    for frame in result['frames']:
        print(f"    Frame {frame['frameNumber']}: {frame['eventDescription']}")

    print("  ✅ Passed")


def test_analysis():
    print("\n=== Test 5: Vulnerability Analysis ===")
    config = gp.GeneratorConfig()
    config.totalNodes = 15
    config.seed = 42
    grid = gp.generateGrid(config)

    # Betweenness
    betweenness = gp.computeBetweenness(grid)
    print(f"  Betweenness — Max edge score: {betweenness['maxEdgeScore']:.4f}")
    print(f"  Betweenness — Max node score: {betweenness['maxNodeScore']:.4f}")
    print(f"  Top 3 critical edges:")
    for i, (edge_id, score) in enumerate(betweenness['edgeScores'][:3]):
        print(f"    Edge {edge_id}: {score:.4f}")

    # Articulation points
    articulation = gp.findArticulationPoints(grid)
    print(f"  Articulation points found: {articulation['articulationCount']}")
    if articulation['articulationPoints']:
        print(f"    Points: {articulation['articulationPoints']}")

    print("  ✅ Passed")


if __name__ == "__main__":
    print("=" * 60)
    print("GridPulse Python Bindings Test Suite")
    print("=" * 60)

    try:
        test_grid_generation()
        test_mcmf()
        test_dijkstra()
        test_cascade()
        test_analysis()

        print("\n" + "=" * 60)
        print(" ALL TESTS PASSED")
        print("=" * 60)
    except Exception as e:
        print(f"\n TEST FAILED: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)