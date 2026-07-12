"""REST endpoints for grid operations."""

from fastapi import APIRouter
from app.models.schemas import GridConfigRequest, GridResponse
from app.services.engine_service import generate_grid

router = APIRouter(prefix="/api/grids", tags=["grids"])


@router.post("/generate", response_model=GridResponse)
async def create_grid(config: GridConfigRequest):
    """Generate a new synthetic power grid."""
    result = generate_grid(config.model_dump())
    return result


@router.post("/flow")
async def compute_flow(config: GridConfigRequest):
    """Compute optimal power flow for a grid configuration."""
    from app.services.engine_service import compute_flow as cf
    result = cf(config.model_dump())
    return result


@router.post("/analyze")
async def analyze_grid(config: GridConfigRequest):
    """Run vulnerability analysis on a grid configuration."""
    from app.services.engine_service import analyze_grid as ag
    result = ag(config.model_dump())
    return result