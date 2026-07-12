"""Pydantic models for API request/response validation."""

from pydantic import BaseModel, Field
from typing import Optional, List
from enum import Enum


class TriggerType(str, Enum):
    CUT_EDGE = "CUT_EDGE"
    FAIL_NODE = "FAIL_NODE"
    SPIKE_DEMAND = "SPIKE_DEMAND"


class GridConfigRequest(BaseModel):
    totalNodes: int = Field(default=40, ge=5, le=200)
    generatorRatio: float = Field(default=0.15, ge=0.05, le=0.5)
    substationRatio: float = Field(default=0.25, ge=0.1, le=0.5)
    redundancyFactor: int = Field(default=5, ge=0, le=50)
    canvasWidth: float = Field(default=800.0)
    canvasHeight: float = Field(default=600.0)
    seed: int = Field(default=42)


class TriggerRequest(BaseModel):
    type: TriggerType = TriggerType.CUT_EDGE
    targetId: int = 0
    value: float = 0.0


class SimulationRequest(BaseModel):
    gridConfig: GridConfigRequest
    trigger: TriggerRequest


class NodeResponse(BaseModel):
    id: int
    type: str
    capacityMW: float
    currentLoadMW: float
    tier: str
    x: float
    y: float


class EdgeResponse(BaseModel):
    id: int
    from_: int = Field(alias="from")
    to: int
    capacityMW: float
    currentFlowMW: float
    resistance: float
    tripped: bool


class FrameResponse(BaseModel):
    frameNumber: int
    eventDescription: str
    edgeFlows: List[float]
    edgeTripped: List[bool]
    islandIds: List[int]
    nodeLoads: List[float]
    nodeShed: List[bool]
    nodes: Optional[List[NodeResponse]] = None
    edges: Optional[List[EdgeResponse]] = None


class CascadeResponse(BaseModel):
    frames: List[FrameResponse]
    totalIterations: int
    gridStabilized: bool
    totalBlackout: bool
    finalDemandServed: float
    initialDemand: float
    totalEdgesTripped: int
    totalNodesShed: int


class GridResponse(BaseModel):
    nodes: List[NodeResponse]
    edges: List[EdgeResponse]


class AnalysisResponse(BaseModel):
    betweenness: dict
    articulationPoints: dict