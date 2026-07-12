"""REST endpoints for simulation operations."""

from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy.orm import Session
from app.models.database import get_db
from app.models.schemas import SimulationRequest, CascadeResponse
from app.services.engine_service import run_cascade
from app.repositories import grid_repo, simulation_repo
from app.services.engine_service import _serialize_grid

router = APIRouter(prefix="/api/simulations", tags=["simulations"])


@router.post("/run")
async def run_simulation(request: SimulationRequest, db: Session = Depends(get_db)):
    """Run a cascade simulation, save grid and results to database."""
    config_dict = request.gridConfig.model_dump()
    trigger_dict = request.trigger.model_dump()
    
    # Generate grid and run cascade
    from app.services.engine_service import generate_grid
    grid_data = generate_grid(config_dict)
    result = run_cascade(config_dict, trigger_dict)
    
    # Save grid to database
    grid_model = grid_repo.save_grid(db, config_dict, grid_data)
    
    # Save simulation
    simulation_repo.save_simulation(db, grid_model.id, trigger_dict, result)
    
    return result


@router.get("/")
async def list_simulations(grid_id: int = None, db: Session = Depends(get_db)):
    """List saved simulations."""
    sims = simulation_repo.list_simulations(db, grid_id)
    return [
        {
            "id": s.id,
            "grid_id": s.grid_id,
            "total_iterations": s.total_iterations,
            "edges_tripped": s.edges_tripped,
            "nodes_shed": s.nodes_shed,
            "grid_stabilized": s.grid_stabilized,
            "created_at": s.created_at.isoformat() if s.created_at else None
        }
        for s in sims
    ]

@router.post("/storm/action")
async def storm_action(request: dict):
    """Handle player actions during a storm."""
    session_id = request.get("sessionId")
    action = request.get("action")
    target_id = request.get("targetId", 0)

    sim = None
    from app.websocket.simulation_ws import active_simulators
    sim = active_simulators.get(session_id)

    if not sim:
        return {"error": "No active storm session"}

    if action == "cutEdge":
        sim.playerCutEdge(target_id)
    elif action == "reinforce":
        success = sim.playerReinforceLine(target_id)
        return {"success": success, "grid": _serialize_grid(sim.getGrid())}
    elif action == "spikeDemand":
        sim.playerSpikeDemand(target_id, request.get("value", 0))

    return {"success": True}

@router.get("/{sim_id}")
async def get_simulation(sim_id: int, db: Session = Depends(get_db)):
    """Get full simulation result by ID (for replay)."""
    import json
    sim = simulation_repo.get_simulation(db, sim_id)
    if not sim:
        raise HTTPException(status_code=404, detail="Simulation not found")
    return json.loads(sim.result_json)