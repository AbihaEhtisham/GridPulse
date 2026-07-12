export interface NodeData {
  id: number;
  type: string;
  capacityMW: number;
  currentLoadMW: number;
  tier: string;
  x: number;
  y: number;
}

export interface EdgeData {
  id: number;
  from: number;
  to: number;
  capacityMW: number;
  currentFlowMW: number;
  resistance: number;
  tripped: boolean;
}

export interface GridData {
  nodes: NodeData[];
  edges: EdgeData[];
}

export interface FrameData {
  frameNumber: number;
  eventDescription: string;
  edgeFlows: number[];
  edgeTripped: boolean[];
  islandIds: number[];
  nodeLoads: number[];
  nodeShed: boolean[];
  nodes: NodeData[];
  edges: EdgeData[];
}

export interface CascadeResult {
  frames: FrameData[];
  totalIterations: number;
  gridStabilized: boolean;
  totalBlackout: boolean;
  finalDemandServed: number;
  initialDemand: number;
  totalEdgesTripped: number;
  totalNodesShed: number;
}

export interface GridConfig {
  totalNodes: number;
  generatorRatio: number;
  substationRatio: number;
  redundancyFactor: number;
  seed: number;
}