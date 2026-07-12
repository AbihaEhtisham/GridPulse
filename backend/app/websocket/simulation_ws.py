"""WebSocket endpoint for live cascade streaming."""

import asyncio
import json
from fastapi import APIRouter, WebSocket, WebSocketDisconnect
from app.models.schemas import SimulationRequest
from app.services.engine_service import run_cascade

router = APIRouter()


@router.websocket("/ws/simulations/run")
async def simulation_websocket(websocket: WebSocket):
    """
    Run a cascade simulation and stream frames in real-time.
    Client sends a SimulationRequest as JSON, then receives frames.
    """
    await websocket.accept()
    
    try:
        # Receive simulation configuration
        data = await websocket.receive_text()
        request_data = json.loads(data)
        
        grid_config = request_data.get("gridConfig", {})
        trigger = request_data.get("trigger", {})
        
        # Run cascade (this runs synchronously but in practice grids are small
        # enough that it completes quickly; for large grids, use run_in_executor)
        result = run_cascade(grid_config, trigger)
        
        # Stream each frame
        frames = result.get("frames", [])
        for i, frame in enumerate(frames):
            message = {
                "type": "frame",
                "frameIndex": i,
                "totalFrames": len(frames),
                "data": frame
            }
            await websocket.send_json(message)
            await asyncio.sleep(0.1)  # small delay for animation feel
        
        # Send completion message with summary
        await websocket.send_json({
            "type": "complete",
            "summary": {
                "totalIterations": result.get("totalIterations"),
                "gridStabilized": result.get("gridStabilized"),
                "totalBlackout": result.get("totalBlackout"),
                "totalEdgesTripped": result.get("totalEdgesTripped"),
                "totalNodesShed": result.get("totalNodesShed"),
                "finalDemandServed": result.get("finalDemandServed"),
                "initialDemand": result.get("initialDemand")
            }
        })
        
    except WebSocketDisconnect:
        pass
    except Exception as e:
        await websocket.send_json({"type": "error", "message": str(e)})