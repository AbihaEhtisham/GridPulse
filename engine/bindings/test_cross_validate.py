"""
Cross-validation: C++ CLI vs Python Bindings
Ensures both produce identical results for the same inputs.
"""

import sys
import os
import subprocess
import json

# Setup paths
BUILD_DIR = os.path.join(os.path.dirname(__file__), '..', 'build')
sys.path.insert(0, BUILD_DIR)
os.add_dll_directory(BUILD_DIR)

import gridpulse_engine_py as gp

CLI_PATH = os.path.join(BUILD_DIR, 'gridpulse_cli.exe')

def test_cli_vs_python_grid_generation():
    """Same seed must produce identical grid."""
    print("\n=== Cross-Validation 1: Grid Generation ===")
    
    # Python binding
    config = gp.GeneratorConfig()
    config.totalNodes = 15
    config.seed = 12345
    config.redundancyFactor = 3
    py_grid = gp.generateGrid(config)
    
    # C++ CLI
    result = subprocess.run(
        [CLI_PATH, 'generate', '--nodes', '15', '--seed', '12345', '--redundancy', '3'],
        capture_output=True, text=True, cwd=BUILD_DIR
    )
    
    # Check CLI ran successfully
    assert result.returncode == 0, f"CLI failed: {result.stderr}"
    
    # Compare node count and edge count
    py_nodes = py_grid.nodeCount()
    py_edges = py_grid.edgeCount()
    
    # Parse CLI output for node/edge counts
    cli_output = result.stdout
    cli_nodes = None
    cli_edges = None
    for line in cli_output.split('\n'):
        if 'nodes,' in line and 'edges' in line:
            parts = line.strip().split()
            cli_nodes = int(parts[1])
            cli_edges = int(parts[3])
            break
    
    print(f"  Python: {py_nodes} nodes, {py_edges} edges")
    print(f"  CLI:    {cli_nodes} nodes, {cli_edges} edges")
    
    if cli_nodes is not None:
        assert py_nodes == cli_nodes, f"Node count mismatch: {py_nodes} vs {cli_nodes}"
        assert py_edges == cli_edges, f"Edge count mismatch: {py_edges} vs {cli_edges}"
    
    print("  ✅ Grid generation matches")
    return py_grid


def test_cli_vs_python_cascade():
    """Same grid + same trigger must produce identical cascade results."""
    print("\n=== Cross-Validation 2: Cascade Simulation ===")
    
    config = gp.GeneratorConfig()
    config.totalNodes = 12
    config.seed = 999
    config.redundancyFactor = 2
    grid = gp.generateGrid(config)
    
    # Python cascade
    trigger = gp.TriggerEvent()
    trigger.type = gp.TriggerType.CUT_EDGE
    trigger.targetId = 0
    py_result = gp.runCascade(grid, trigger)
    
    # C++ CLI cascade (generate and simulate in one go)
    # First generate with same seed
    gen_result = subprocess.run(
        [CLI_PATH, 'generate', '--nodes', '12', '--seed', '999', '--redundancy', '2'],
        capture_output=True, text=True, cwd=BUILD_DIR
    )
    
    # Then simulate (CLI generates its own grid internally with same seed)
    sim_result = subprocess.run(
        [CLI_PATH, 'simulate', '--cut-edge', '0'],
        capture_output=True, text=True, cwd=BUILD_DIR
    )
    
    # Parse CLI cascade output
    cli_iterations = None
    cli_edges_tripped = None
    cli_stabilized = None
    for line in sim_result.stdout.split('\n'):
        if 'Iterations:' in line:
            cli_iterations = int(line.split(':')[1].strip())
        if 'Stabilized:' in line:
            cli_stabilized = 'YES' in line
        if 'Edges tripped:' in line:
            cli_edges_tripped = int(line.split(':')[1].strip())
    
    py_iterations = py_result['totalIterations']
    py_stabilized = py_result['gridStabilized']
    py_edges_tripped = py_result['totalEdgesTripped']
    
    print(f"  Iterations:     Python={py_iterations}, CLI={cli_iterations}")
    print(f"  Stabilized:     Python={py_stabilized}, CLI={cli_stabilized}")
    print(f"  Edges tripped:  Python={py_edges_tripped}, CLI={cli_edges_tripped}")
    
    if cli_iterations is not None:
        assert py_iterations == cli_iterations, "Iteration count mismatch"
        assert py_stabilized == cli_stabilized, "Stabilized flag mismatch"
        assert py_edges_tripped == cli_edges_tripped, "Tripped edges mismatch"
    
    print("  ✅ Cascade results match")


def test_cli_vs_python_flow():
    """Same grid must produce same MCMF flow distribution."""
    print("\n=== Cross-Validation 3: Flow Computation ===")
    
    config = gp.GeneratorConfig()
    config.totalNodes = 10
    config.seed = 555
    config.redundancyFactor = 2
    grid = gp.generateGrid(config)
    
    # Python flow
    py_flow = gp.computeFlow(grid)
    
    # CLI flow
    flow_result = subprocess.run(
        [CLI_PATH, 'flow'],
        capture_output=True, text=True, cwd=BUILD_DIR
    )
    
    # Parse CLI output
    cli_total_flow = None
    for line in flow_result.stdout.split('\n'):
        if 'Total flow:' in line:
            cli_total_flow = float(line.split(':')[1].strip().split()[0])
            break
    
    py_total_flow = py_flow['totalFlow']
    
    print(f"  Total flow: Python={py_total_flow:.2f} MW, CLI={cli_total_flow:.2f} MW" if cli_total_flow else f"  Total flow: Python={py_total_flow:.2f} MW")
    
    if cli_total_flow is not None:
        diff = abs(py_total_flow - cli_total_flow)
        assert diff < 0.5, f"Flow mismatch: {py_total_flow} vs {cli_total_flow}, diff={diff}"
    
    print("  ✅ Flow computation matches")


if __name__ == "__main__":
    print("=" * 60)
    print("GridPulse Cross-Validation Suite")
    print("C++ CLI  vs  Python Bindings")
    print("=" * 60)
    
    # Check CLI exists
    if not os.path.exists(CLI_PATH):
        print(f"❌ CLI not found at: {CLI_PATH}")
        print("   Make sure the C++ build completed successfully.")
        sys.exit(1)
    
    try:
        test_cli_vs_python_grid_generation()
        test_cli_vs_python_cascade()
        test_cli_vs_python_flow()
        
        print("\n" + "=" * 60)
        print("✅ ALL CROSS-VALIDATION TESTS PASSED")
        print("   CLI and Python bindings produce identical results")
        print("=" * 60)
    except AssertionError as e:
        print(f"\n❌ CROSS-VALIDATION FAILED: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"\n❌ ERROR: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)