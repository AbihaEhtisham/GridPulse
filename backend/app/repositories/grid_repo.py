"""Repository for grid persistence operations."""

import json
from sqlalchemy.orm import Session
from app.models.orm import GridModel


def save_grid(db: Session, config: dict, grid_data: dict, name: str = "Unnamed Grid") -> GridModel:
    """Save a grid to the database."""
    grid_model = GridModel(
        name=name,
        config_json=json.dumps(config),
        grid_data_json=json.dumps(grid_data)
    )
    db.add(grid_model)
    db.commit()
    db.refresh(grid_model)
    return grid_model


def get_grid(db: Session, grid_id: int) -> GridModel | None:
    """Retrieve a grid by ID."""
    return db.query(GridModel).filter(GridModel.id == grid_id).first()


def list_grids(db: Session, limit: int = 20) -> list[GridModel]:
    """List recent grids."""
    return db.query(GridModel).order_by(GridModel.created_at.desc()).limit(limit).all()


def delete_grid(db: Session, grid_id: int) -> bool:
    """Delete a grid and its simulations."""
    grid = db.query(GridModel).filter(GridModel.id == grid_id).first()
    if grid:
        db.delete(grid)
        db.commit()
        return True
    return False