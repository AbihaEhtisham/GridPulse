"""Repository for simulation persistence operations."""

import json
from sqlalchemy.orm import Session
from app.models.orm import SimulationModel, GridModel


def save_simulation(
    db: Session,
    grid_id: int,
    trigger: dict,
    result: dict
) -> SimulationModel:
    """Save a simulation run to the database."""
    sim_model = SimulationModel(
        grid_id=grid_id,
        trigger_json=json.dumps(trigger),
        result_json=json.dumps(result),
        total_iterations=result.get("totalIterations", 0),
        edges_tripped=result.get("totalEdgesTripped", 0),
        nodes_shed=result.get("totalNodesShed", 0),
        grid_stabilized=result.get("gridStabilized", False)
    )
    db.add(sim_model)
    db.commit()
    db.refresh(sim_model)
    return sim_model


def get_simulation(db: Session, sim_id: int) -> SimulationModel | None:
    """Retrieve a simulation by ID."""
    return db.query(SimulationModel).filter(SimulationModel.id == sim_id).first()


def list_simulations(db: Session, grid_id: int = None, limit: int = 20) -> list[SimulationModel]:
    """List simulations, optionally filtered by grid."""
    query = db.query(SimulationModel)
    if grid_id is not None:
        query = query.filter(SimulationModel.grid_id == grid_id)
    return query.order_by(SimulationModel.created_at.desc()).limit(limit).all()