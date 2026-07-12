"""ORM models for database tables."""

from sqlalchemy import Column, Integer, Float, String, Boolean, Text, DateTime, ForeignKey, JSON
from sqlalchemy.orm import relationship
from datetime import datetime, timezone
from app.models.database import Base


class GridModel(Base):
    __tablename__ = "grids"

    id = Column(Integer, primary_key=True, autoincrement=True)
    name = Column(String(255), nullable=False, default="Unnamed Grid")
    config_json = Column(Text, nullable=False)       # GridConfig as JSON
    grid_data_json = Column(Text, nullable=False)     # Serialized grid (nodes + edges)
    created_at = Column(DateTime, default=lambda: datetime.now(timezone.utc))

    simulations = relationship("SimulationModel", back_populates="grid", cascade="all, delete-orphan")


class SimulationModel(Base):
    __tablename__ = "simulations"

    id = Column(Integer, primary_key=True, autoincrement=True)
    grid_id = Column(Integer, ForeignKey("grids.id"), nullable=False)
    trigger_json = Column(Text, nullable=False)       # TriggerEvent as JSON
    result_json = Column(Text, nullable=False)         # Full cascade result as JSON
    total_iterations = Column(Integer, default=0)
    edges_tripped = Column(Integer, default=0)
    nodes_shed = Column(Integer, default=0)
    grid_stabilized = Column(Boolean, default=False)
    created_at = Column(DateTime, default=lambda: datetime.now(timezone.utc))

    grid = relationship("GridModel", back_populates="simulations")