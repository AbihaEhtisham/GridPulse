"""WebSocket endpoint for live storm simulation streaming."""

import json
import asyncio
from fastapi import APIRouter, WebSocket, WebSocketDisconnect
from app.services.engine_service import init_storm_simulator, _serialize_grid

router = APIRouter()
active_simulators = {}


@router.websocket("/ws/storm/{session_id}")
async def storm_websocket(websocket: WebSocket, session_id: str):
    await websocket.accept()

    try:
        data = await websocket.receive_text()
        request = json.loads(data)

        grid_config = request.get("gridConfig", {})
        storm_config = request.get("stormConfig", {})

        grid_data, sim = init_storm_simulator(grid_config, storm_config)
        active_simulators[session_id] = sim

        # Send initial grid
        await websocket.send_json({
            "type": "init",
            "grid": grid_data,
            "budget": storm_config.get("playerBudget", 100.0)
        })

        # Stream storm events one by one with a small delay for animation
        step = 0
        while not sim.isFinished() and step < 200:
            frame = sim.stepNext()
            grid_data = _serialize_grid(sim.getGrid())

            await websocket.send_json({
                "type": "frame",
                "step": step,
                "timestamp": frame.timestamp,
                "description": frame.eventDescription,
                "gridHealth": frame.gridHealth,
                "criticalProtected": frame.criticalLoadProtected,
                "linesFailed": frame.linesFailed,
                "grid": grid_data
            })

            step += 1
            await asyncio.sleep(0.3)  # 300ms delay between events for animation

        # Send completion
        await websocket.send_json({
            "type": "complete",
            "gridHealth": 0,
            "message": "Storm has passed! The city survived."
        })

    except WebSocketDisconnect:
        pass
    except Exception as e:
        await websocket.send_json({"type": "error", "message": str(e)})