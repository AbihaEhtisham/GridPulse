import { useState, useCallback, useRef } from 'react';
import GridCanvas from './components/GridCanvas';
import { generateMapGrid, runSimulation } from './services/api';
import type { GridData, CascadeResult, FrameData } from './types/grid';

export default function App() {
  const [gridData, setGridData] = useState<GridData | null>(null);
  const [frames, setFrames] = useState<FrameData[]>([]);
  const [currentFrame, setCurrentFrame] = useState(0);
  const [playing, setPlaying] = useState(false);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [nodeCount, setNodeCount] = useState(25);
  const [seed, setSeed] = useState(42);
  const [mode, setMode] = useState<'view' | 'cutEdge' | 'spikeDemand'>('view');
  const [showHeatmap, setShowHeatmap] = useState(false);
  const [summary, setSummary] = useState<CascadeResult | null>(null);
  const playIntervalRef = useRef<number | null>(null);

  // Generate a fresh grid

const handleGenerate = useCallback(async () => {
  setLoading(true);
  setError(null);
  setFrames([]);
  setCurrentFrame(0);
  setSummary(null);
  setShowHeatmap(false);
  try {
    const data = await generateMapGrid({
      mapWidth: 1200,
      mapHeight: 800,
      numPowerPlants: 3,
      numSubstations: 5,
      numHospitals: 2,
      numCommercial: 8,
      numResidential: nodeCount,
      redundancyFactor: 5,
      seed,
    });
    setGridData(data);
  } catch (err: any) {
    setError(err?.message || 'Failed to generate grid');
  }
  setLoading(false);
}, [nodeCount, seed]);

  // Run cascade and collect all frames
  const handleRunCascade = useCallback(async () => {
    if (!gridData) return;
    setLoading(true);
    setError(null);
    try {
      const result = await runSimulation(
        { totalNodes: nodeCount, generatorRatio: 0.2, substationRatio: 0.25, redundancyFactor: 3, seed },
        { type: 'CUT_EDGE', targetId: 0 }
      );
      setFrames(result.frames);
      setSummary(result);
      setCurrentFrame(0);
      // Apply first frame
      applyFrame(result.frames[0]);
    } catch (err: any) {
      setError(err?.message || 'Simulation failed');
    }
    setLoading(false);
  }, [gridData, nodeCount, seed]);

  // Apply a specific frame to the grid
  const applyFrame = useCallback(
    (frame: FrameData) => {
      if (!frame || !gridData) return;
      setGridData((prev) => {
        if (!prev) return prev;
        return {
          nodes: (frame.nodes || prev.nodes).map((n) => ({
            ...n,
            currentLoadMW: frame.nodeLoads?.[n.id] ?? n.currentLoadMW,
          })),
          edges: (frame.edges || prev.edges).map((e, i) => ({
            ...e,
            currentFlowMW: frame.edgeFlows?.[i] ?? 0,
            tripped: frame.edgeTripped?.[i] ?? false,
          })),
        };
      });
    },
    [gridData]
  );

  // Jump to a specific frame
  const goToFrame = useCallback(
    (index: number) => {
      const i = Math.max(0, Math.min(index, frames.length - 1));
      setCurrentFrame(i);
      applyFrame(frames[i]);
    },
    [frames, applyFrame]
  );

  // Play/Pause animation
  const togglePlay = useCallback(() => {
    if (playing) {
      if (playIntervalRef.current) clearInterval(playIntervalRef.current);
      setPlaying(false);
    } else {
      setPlaying(true);
      let frame = currentFrame;
      playIntervalRef.current = window.setInterval(() => {
        frame++;
        if (frame >= frames.length) {
          frame = frames.length - 1;
          if (playIntervalRef.current) clearInterval(playIntervalRef.current);
          setPlaying(false);
        }
        goToFrame(frame);
      }, 600);
    }
  }, [playing, currentFrame, frames, goToFrame]);

  // Handle edge click
  const handleEdgeClick = useCallback(
    async (edgeId: number) => {
      if (mode === 'cutEdge' && gridData) {
        setLoading(true);
        try {
          const result = await runSimulation(
            { totalNodes: nodeCount, generatorRatio: 0.2, substationRatio: 0.25, redundancyFactor: 3, seed },
            { type: 'CUT_EDGE', targetId: edgeId }
          );
          setFrames(result.frames);
          setSummary(result);
          setCurrentFrame(0);
          applyFrame(result.frames[0]);
        } catch (err: any) {
          setError(err?.message || 'Cut edge failed');
        }
        setLoading(false);
        setMode('view');
      }
    },
    [mode, gridData, nodeCount, seed, applyFrame]
  );

  const handleNodeClick = useCallback(
    async (nodeId: number) => {
      if (mode === 'spikeDemand' && gridData) {
        setLoading(true);
        try {
          const result = await runSimulation(
            { totalNodes: nodeCount, generatorRatio: 0.2, substationRatio: 0.25, redundancyFactor: 3, seed },
            { type: 'SPIKE_DEMAND', targetId: nodeId}
          );
          setFrames(result.frames);
          setSummary(result);
          setCurrentFrame(0);
          applyFrame(result.frames[0]);
        } catch (err: any) {
          setError(err?.message || 'Spike demand failed');
        }
        setLoading(false);
        setMode('view');
      }
    },
    [mode, gridData, nodeCount, seed, applyFrame]
  );

  const currentFrameData = frames[currentFrame];

  return (
    <div className="h-screen flex flex-col bg-slate-950 text-slate-200 overflow-hidden">
      {/* Top Bar */}
      <header className="shrink-0 border-b border-slate-800 bg-slate-900 px-6 py-3 flex items-center justify-between">
        <div className="flex items-center gap-3">
          <span className="text-2xl">⚡</span>
          <div>
            <h1 className="text-lg font-bold text-white">GridPulse</h1>
            <p className="text-xs text-slate-500">Power Grid Resilience Platform</p>
          </div>
        </div>

        <div className="flex items-center gap-2">
          <button
            onClick={handleGenerate}
            disabled={loading}
            className="px-4 py-2 bg-blue-600 hover:bg-blue-500 disabled:opacity-40 rounded-lg text-sm font-semibold transition"
          >
            Generate Grid
          </button>
          <button
            onClick={handleRunCascade}
            disabled={!gridData || loading}
            className="px-4 py-2 bg-red-600 hover:bg-red-500 disabled:opacity-40 rounded-lg text-sm font-semibold transition"
          >
            Run Cascade
          </button>
          <button
            onClick={() => setShowHeatmap(!showHeatmap)}
            disabled={!gridData}
            className={`px-4 py-2 rounded-lg text-sm font-semibold transition ${
              showHeatmap ? 'bg-yellow-600 text-white' : 'bg-slate-800 text-slate-400 hover:bg-slate-700'
            }`}
          >
            Heatmap
          </button>
        </div>

        <div className="flex items-center gap-3">
          <span className="text-xs text-slate-500">Nodes:</span>
          <input
            type="number"
            value={nodeCount}
            onChange={(e) => setNodeCount(Number(e.target.value))}
            className="w-16 bg-slate-800 border border-slate-700 rounded px-2 py-1 text-sm text-white text-center"
          />
          <button onClick={() => setSeed(Math.floor(Math.random() * 10000))} className="text-sm px-2 py-1 bg-slate-800 hover:bg-slate-700 rounded">
            🎲
          </button>
        </div>
      </header>

      {/* Toolbar */}
      <div className="shrink-0 border-b border-slate-800 bg-slate-900/50 px-6 py-2 flex items-center gap-3">
        <span className="text-xs text-slate-500 uppercase tracking-wider">Tools:</span>
        {(['view', 'cutEdge', 'spikeDemand'] as const).map((m) => (
          <button
            key={m}
            onClick={() => setMode(m)}
            className={`px-3 py-1.5 rounded-lg text-xs font-semibold transition ${
              mode === m
                ? 'bg-white/10 text-white border border-white/20'
                : 'bg-transparent text-slate-500 hover:text-slate-300 border border-transparent'
            }`}
          >
            {m === 'view' ? '👆 View' : m === 'cutEdge' ? '✂️ Cut Edge' : '📈 Spike Demand'}
          </button>
        ))}
        {mode !== 'view' && (
          <span className="text-xs text-yellow-400 animate-pulse">
            Click on a {mode === 'cutEdge' ? 'line' : 'node'} in the grid
          </span>
        )}
        {loading && <span className="text-xs text-blue-400 animate-pulse ml-auto">Processing...</span>}
      </div>

      {/* Main Content */}
      <div className="flex-1 flex overflow-hidden">
        {/* Grid Canvas */}
        <div className="flex-1 p-4">
          <div className="h-full bg-slate-900/50 border border-slate-800 rounded-xl overflow-hidden">
            {gridData ? (
              <GridCanvas
                gridData={gridData}
                onEdgeClick={handleEdgeClick}
                onNodeClick={handleNodeClick}
                highlightMode={mode}
                showHeatmap={showHeatmap}
              />
            ) : (
              <div className="h-full flex items-center justify-center text-slate-600">
                <div className="text-center">
                  <div className="text-5xl mb-4">⚡</div>
                  <p className="text-lg">Generate a grid to begin</p>
                  <p className="text-sm mt-1">Configure node count and click Generate Grid</p>
                </div>
              </div>
            )}
          </div>
        </div>

        {/* Right Panel */}
        <aside className="w-80 shrink-0 border-l border-slate-800 bg-slate-900/50 p-4 flex flex-col gap-4 overflow-y-auto">
          {/* Live Stats */}
          <div className="bg-slate-900 border border-slate-800 rounded-xl p-4">
            <h3 className="text-xs font-semibold text-slate-400 uppercase tracking-wider mb-3">Live Metrics</h3>
            {gridData ? (
              <div className="space-y-2 text-sm">
                <Stat label="Active Nodes" value={String(gridData.nodes.length)} />
                <Stat label="Active Edges" value={String(gridData.edges.filter((e) => !e.tripped).length)} />
                <Stat label="Tripped Edges" value={String(gridData.edges.filter((e) => e.tripped).length)} color="text-red-400" />
              </div>
            ) : (
              <p className="text-xs text-slate-600">No grid loaded</p>
            )}
          </div>

          {/* Cascade Frame Info */}
          {currentFrameData && (
            <div className="bg-slate-900 border border-slate-800 rounded-xl p-4">
              <h3 className="text-xs font-semibold text-slate-400 uppercase tracking-wider mb-3">
                Frame {currentFrame + 1} / {frames.length}
              </h3>
              <p className="text-sm text-slate-300">{currentFrameData.eventDescription}</p>
              <div className="mt-2 text-xs text-slate-500">
                Island groups: {new Set(currentFrameData.islandIds).size}
              </div>
            </div>
          )}

          {/* Summary */}
          {summary && (
            <div className="bg-slate-900 border border-slate-800 rounded-xl p-4">
              <h3 className="text-xs font-semibold text-slate-400 uppercase tracking-wider mb-3">Result</h3>
              <div className="space-y-2 text-sm">
                <Stat label="Stabilized" value={summary.gridStabilized ? '✅ Yes' : '❌ No'} />
                <Stat label="Edges Tripped" value={String(summary.totalEdgesTripped)} color="text-red-400" />
                <Stat label="Loads Shed" value={String(summary.totalNodesShed)} color="text-orange-400" />
                <Stat
                  label="Demand Served"
                  value={`${summary.initialDemand > 0 ? ((summary.finalDemandServed / summary.initialDemand) * 100).toFixed(0) : 0}%`}
                  color="text-green-400"
                />
              </div>
            </div>
          )}

          {error && (
            <div className="bg-red-900/30 border border-red-800 rounded-xl p-4 text-sm text-red-400">
              {error}
            </div>
          )}
        </aside>
      </div>

      {/* Timeline */}
      {frames.length > 0 && (
        <div className="shrink-0 border-t border-slate-800 bg-slate-900 px-6 py-3">
          <div className="flex items-center gap-4">
            <button onClick={togglePlay} className="text-lg w-8 h-8 flex items-center justify-center bg-slate-800 hover:bg-slate-700 rounded-lg transition">
              {playing ? '⏸' : '▶'}
            </button>
            <button onClick={() => goToFrame(0)} className="text-sm px-2 py-1 bg-slate-800 hover:bg-slate-700 rounded">⏮</button>
            <button onClick={() => goToFrame(currentFrame - 1)} className="text-sm px-2 py-1 bg-slate-800 hover:bg-slate-700 rounded">◀</button>
            <input
              type="range"
              min={0}
              max={frames.length - 1}
              value={currentFrame}
              onChange={(e) => goToFrame(Number(e.target.value))}
              className="flex-1 accent-blue-500"
            />
            <button onClick={() => goToFrame(currentFrame + 1)} className="text-sm px-2 py-1 bg-slate-800 hover:bg-slate-700 rounded">▶</button>
            <button onClick={() => goToFrame(frames.length - 1)} className="text-sm px-2 py-1 bg-slate-800 hover:bg-slate-700 rounded">⏭</button>
            <span className="text-xs text-slate-500 font-mono w-24 text-right">
              {currentFrame + 1} / {frames.length}
            </span>
          </div>
        </div>
      )}
    </div>
  );
}

function Stat({ label, value, color = 'text-white' }: { label: string; value: string; color?: string }) {
  return (
    <div className="flex justify-between">
      <span className="text-slate-500">{label}</span>
      <span className={`font-mono font-semibold ${color}`}>{value}</span>
    </div>
  );
}