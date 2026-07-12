"""REST endpoints for simulation operations."""

from fastapi import APIRouter
from app.models.schemas import SimulationRequest, CascadeResponse
from app.services.engine_service import run_cascade

router = APIRouter(prefix="/api/simulations", tags=["simulations"])


@router.post("/run", response_model=CascadeResponse)
async def run_simulation(request: SimulationRequest):
    """Run a cascade simulation and return all frames."""
    result = run_cascade(
        request.gridConfig.model_dump(),
        request.trigger.model_dump()
    )
    return result
