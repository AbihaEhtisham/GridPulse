import axios from 'axios';
import type { GridData, CascadeResult, GridConfig } from '../types/grid';

const API_BASE = 'http://localhost:8000/api';

export async function generateGrid(config: GridConfig): Promise<GridData> {
  const response = await axios.post(`${API_BASE}/grids/generate`, config);
  return response.data;
}

export async function generateMapGrid(config: Record<string, number>): Promise<GridData> {
  const response = await axios.post(`${API_BASE}/grids/generate-map`, config);
  return response.data;
}

export async function runSimulation(
  gridConfig: GridConfig,
  trigger: { type: string; targetId: number }
): Promise<CascadeResult> {
  const response = await axios.post(`${API_BASE}/simulations/run`, {
    gridConfig,
    trigger,
  });
  return response.data;
}

export async function getGrid(gridId: number): Promise<GridData> {
  const response = await axios.get(`${API_BASE}/grids/${gridId}`);
  return response.data;
}

export async function listGrids(): Promise<{ id: number; name: string }[]> {
  const response = await axios.get(`${API_BASE}/grids/`);
  return response.data;
}

export async function getSimulation(simId: number): Promise<CascadeResult> {
  const response = await axios.get(`${API_BASE}/simulations/${simId}`);
  return response.data;
}