"""REST endpoints for grid operations."""

from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy.orm import Session
from app.models.database import get_db
from app.models.schemas import GridConfigRequest, GridResponse
from app.services.engine_service import generate_grid
from app.repositories import grid_repo

router = APIRouter(prefix="/api/grids", tags=["grids"])


@router.post("/generate", response_model=GridResponse)
async def create_grid(config: GridConfigRequest, db: Session = Depends(get_db)):
    """Generate a new synthetic power grid and save to database."""
    config_dict = config.model_dump()
    grid_data = generate_grid(config_dict)
    
    # Save to database
    grid_repo.save_grid(db, config_dict, grid_data, name=f"Grid_{config.totalNodes}nodes")
    
    return grid_data


@router.get("/")
async def list_grids(db: Session = Depends(get_db)):
    """List all saved grids."""
    grids = grid_repo.list_grids(db)
    return [
        {
            "id": g.id,
            "name": g.name,
            "created_at": g.created_at.isoformat() if g.created_at else None
        }
        for g in grids
    ]


@router.get("/{grid_id}")
async def get_grid(grid_id: int, db: Session = Depends(get_db)):
    """Get a specific grid by ID."""
    import json
    grid = grid_repo.get_grid(db, grid_id)
    if not grid:
        raise HTTPException(status_code=404, detail="Grid not found")
    return json.loads(grid.grid_data_json)


@router.delete("/{grid_id}")
async def delete_grid(grid_id: int, db: Session = Depends(get_db)):
    """Delete a grid and its simulations."""
    success = grid_repo.delete_grid(db, grid_id)
    if not success:
        raise HTTPException(status_code=404, detail="Grid not found")
    return {"status": "deleted", "grid_id": grid_id}


@router.post("/flow")
async def compute_flow(config: GridConfigRequest):
    """Compute optimal power flow for a grid configuration."""
    from app.services.engine_service import compute_flow as cf
    return cf(config.model_dump())


@router.post("/analyze")
async def analyze_grid(config: GridConfigRequest):
    """Run vulnerability analysis on a grid configuration."""
    from app.services.engine_service import analyze_grid as ag
    return ag(config.model_dump())