"""GridPulse Backend — FastAPI Application."""

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from app.routes import grids, simulations
from app.websocket import simulation_ws

app = FastAPI(
    title="GridPulse API",
    description="Intelligent Decision Support Platform for Power Grid Resilience",
    version="1.0.0"
)

# CORS — allow frontend dev server
app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:5173", "http://127.0.0.1:5173"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Register routes
app.include_router(grids.router)
app.include_router(simulations.router)
app.include_router(simulation_ws.router)


@app.get("/health")
async def health_check():
    return {"status": "ok", "message": "GridPulse backend is running"}