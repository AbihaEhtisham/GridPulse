import { useState, useCallback, useRef, useEffect } from 'react';
import GridCanvas from './components/GridCanvas';
import { generateMapGrid } from './services/api';
import type { GridData } from './types/grid';

const WS_URL = 'ws://localhost:8000/ws/storm';

export default function App() {
  const [gridData, setGridData] = useState<GridData | null>(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [nodeCount, setNodeCount] = useState(20);
  const [seed, setSeed] = useState(42);
  const [stormIntensity, setStormIntensity] = useState(0.6);
  const [mode, setMode] = useState<'view' | 'cutEdge' | 'reinforce'>('view');
  const [showHeatmap, setShowHeatmap] = useState(false);

  // Storm state
  const [stormActive, setStormActive] = useState(false);
  const [gridHealth, setGridHealth] = useState(100);
  const [criticalProtected, setCriticalProtected] = useState(100);
  const [linesFailed, setLinesFailed] = useState(0);
  const [budget, setBudget] = useState(100);
  const [eventLog, setEventLog] = useState<string[]>([]);
  const [stormProgress, setStormProgress] = useState(0);
  const wsRef = useRef<WebSocket | null>(null);

  // Generate map grid
  const handleGenerate = useCallback(async () => {
    setLoading(true);
    setError(null);
    setStormActive(false);
    setEventLog([]);
    setLinesFailed(0);
    setGridHealth(100);
    setCriticalProtected(100);
    setStormProgress(0);
    try {
      const data = await generateMapGrid({
        mapWidth: 1200, mapHeight: 800,
        numPowerPlants: 3, numSubstations: 5,
        numHospitals: 2, numCommercial: 8,
        numResidential: nodeCount,
        redundancyFactor: 5, seed,
      });
      setGridData(data);
    } catch (err: any) {
      setError(err?.message || 'Failed to generate grid');
    }
    setLoading(false);
  }, [nodeCount, seed]);

  // Start storm via WebSocket
  const handleStartStorm = useCallback(() => {
    if (!gridData || stormActive) return;
    setStormActive(true);
    setEventLog(['⚠️ Storm approaching the city...']);

    const sessionId = `storm_${Date.now()}`;
    const ws = new WebSocket(`${WS_URL}/${sessionId}`);
    wsRef.current = ws;

    ws.onopen = () => {
      ws.send(JSON.stringify({
        gridConfig: {
          mapWidth: 1200, mapHeight: 800,
          numPowerPlants: 3, numSubstations: 5,
          numHospitals: 2, numCommercial: 8,
          numResidential: nodeCount,
          redundancyFactor: 5, seed,
        },
        stormConfig: {
          duration: 120,
          intensity: stormIntensity,
          numInitialFailures: 3,
          seed,
          playerBudget: 100,
        }
      }));
    };

    ws.onmessage = (event) => {
      const msg = JSON.parse(event.data);

      if (msg.type === 'init') {
        setGridData(msg.grid);
        setBudget(msg.budget);
      } else if (msg.type === 'frame') {
        setGridData(msg.grid);
        setGridHealth(Math.round(msg.gridHealth * 100));
        setCriticalProtected(Math.round(msg.criticalProtected * 100));
        setLinesFailed(msg.linesFailed);
        setStormProgress(Math.round((msg.timestamp / 120) * 100));
        setEventLog(prev => [...prev.slice(-19), `${msg.description}`]);
      } else if (msg.type === 'complete') {
        setStormActive(false);
        setEventLog(prev => [...prev.slice(-19), '✅ Storm has passed!']);
      }
    };

    ws.onerror = () => setError('WebSocket connection failed');
    ws.onclose = () => setStormActive(false);
  }, [gridData, stormActive, nodeCount, seed, stormIntensity]);

  // Player actions
  const handleEdgeClick = useCallback((edgeId: number) => {
    if (!stormActive || !wsRef.current) return;
    if (mode === 'cutEdge') {
      wsRef.current.send(JSON.stringify({
        sessionId: '', action: 'cutEdge', targetId: edgeId
      }));
      setMode('view');
    } else if (mode === 'reinforce') {
      wsRef.current.send(JSON.stringify({
        sessionId: '', action: 'reinforce', targetId: edgeId
      }));
      setBudget(prev => prev - 20);
      setMode('view');
    }
  }, [stormActive, mode]);

  return (
    <div className="h-screen flex flex-col bg-slate-950 text-slate-200 overflow-hidden">
      {/* Header */}
      <header className="shrink-0 border-b border-slate-800 bg-slate-900 px-6 py-3 flex items-center justify-between">
        <div className="flex items-center gap-3">
          <span className="text-2xl">⛈️</span>
          <div>
            <h1 className="text-lg font-bold text-white">GridPulse: Storm Watch</h1>
            <p className="text-xs text-slate-500">Protect the city from the approaching storm</p>
          </div>
        </div>

        <div className="flex items-center gap-2">
          <span className="text-xs text-slate-500">Houses:</span>
          <input type="number" value={nodeCount} onChange={(e) => setNodeCount(Number(e.target.value))}
            className="w-16 bg-slate-800 border border-slate-700 rounded px-2 py-1 text-sm text-white text-center" />
          <span className="text-xs text-slate-500">Storm:</span>
          <input type="range" min="0.2" max="1.0" step="0.1" value={stormIntensity}
            onChange={(e) => setStormIntensity(Number(e.target.value))}
            className="w-24 accent-orange-500" />
          <span className="text-xs text-orange-400">{Math.round(stormIntensity * 100)}%</span>
          <button onClick={() => setSeed(Math.floor(Math.random() * 10000))}
            className="text-sm px-2 py-1 bg-slate-800 hover:bg-slate-700 rounded">🎲</button>
        </div>
      </header>

      {/* Toolbar */}
      <div className="shrink-0 border-b border-slate-800 bg-slate-900/50 px-6 py-2 flex items-center gap-3">
        <button onClick={handleGenerate} disabled={loading}
          className="px-4 py-2 bg-blue-600 hover:bg-blue-500 disabled:opacity-40 rounded-lg text-sm font-semibold transition">
          🏗️ Build City
        </button>
        <button onClick={handleStartStorm} disabled={!gridData || stormActive}
          className="px-4 py-2 bg-orange-600 hover:bg-orange-500 disabled:opacity-40 rounded-lg text-sm font-semibold transition animate-pulse">
          🌪️ Start Storm
        </button>

        <div className="w-px h-6 bg-slate-700 mx-2" />

        <span className="text-xs text-slate-500 uppercase tracking-wider">Tools:</span>
        {(['view', 'cutEdge', 'reinforce'] as const).map((m) => (
          <button key={m} onClick={() => setMode(m)}
            className={`px-3 py-1.5 rounded-lg text-xs font-semibold transition ${
              mode === m ? 'bg-white/10 text-white border border-white/20'
                : 'bg-transparent text-slate-500 hover:text-slate-300 border border-transparent'
            }`}>
            {m === 'view' ? '👆 View' : m === 'cutEdge' ? '✂️ Cut Line' : '🛡️ Reinforce ($20)'}
          </button>
        ))}
        <button onClick={() => setShowHeatmap(!showHeatmap)}
          className={`px-3 py-1.5 rounded-lg text-xs font-semibold transition ${
            showHeatmap ? 'bg-yellow-600 text-white' : 'bg-slate-800 text-slate-400'
          }`}>🔥 Heatmap</button>

        {loading && <span className="text-xs text-blue-400 animate-pulse ml-auto">Processing...</span>}
      </div>

      {/* Health Bars */}
      {gridData && (
        <div className="shrink-0 px-6 py-2 bg-slate-900/30 border-b border-slate-800 flex gap-6">
          <div className="flex-1">
            <div className="flex justify-between text-xs mb-1">
              <span className="text-slate-400">Grid Health</span>
              <span className={gridHealth > 70 ? 'text-green-400' : gridHealth > 30 ? 'text-yellow-400' : 'text-red-400'}>{gridHealth}%</span>
            </div>
            <div className="w-full h-2.5 bg-slate-800 rounded-full overflow-hidden">
              <div className="h-full bg-gradient-to-r from-green-500 to-emerald-400 rounded-full transition-all duration-500"
                style={{ width: `${gridHealth}%` }} />
            </div>
          </div>
          <div className="flex-1">
            <div className="flex justify-between text-xs mb-1">
              <span className="text-slate-400">Critical Protected</span>
              <span className="text-blue-400">{criticalProtected}%</span>
            </div>
            <div className="w-full h-2.5 bg-slate-800 rounded-full overflow-hidden">
              <div className="h-full bg-gradient-to-r from-blue-500 to-cyan-400 rounded-full transition-all duration-500"
                style={{ width: `${criticalProtected}%` }} />
            </div>
          </div>
          <div className="flex-1">
            <div className="flex justify-between text-xs mb-1">
              <span className="text-slate-400">Storm Progress</span>
              <span className="text-orange-400">{stormProgress}%</span>
            </div>
            <div className="w-full h-2.5 bg-slate-800 rounded-full overflow-hidden">
              <div className="h-full bg-gradient-to-r from-orange-500 to-red-500 rounded-full transition-all duration-500"
                style={{ width: `${stormProgress}%` }} />
            </div>
          </div>
          <div className="text-xs text-slate-400 flex items-center gap-2">
            <span>💰 Budget:</span>
            <span className="text-yellow-400 font-mono font-bold">${budget}</span>
          </div>
          <div className="text-xs text-slate-400 flex items-center gap-2">
            <span>🔌 Lines Down:</span>
            <span className="text-red-400 font-mono font-bold">{linesFailed}</span>
          </div>
        </div>
      )}

      {/* Main Content */}
      <div className="flex-1 flex overflow-hidden">
        {/* Grid Canvas */}
        <div className="flex-1 p-4">
          <div className="h-full bg-slate-900/50 border border-slate-800 rounded-xl overflow-hidden">
            {gridData ? (
              <GridCanvas gridData={gridData} onEdgeClick={handleEdgeClick}
                highlightMode={mode === 'view' ? 'view' : mode === 'cutEdge' ? 'cutEdge' : 'view'}
                showHeatmap={showHeatmap} />
            ) : (
              <div className="h-full flex items-center justify-center text-slate-600">
                <div className="text-center">
                  <div className="text-6xl mb-4">🏙️</div>
                  <p className="text-lg font-semibold">GridPulse City</p>
                  <p className="text-sm mt-1">Build a city and defend it against the storm</p>
                </div>
              </div>
            )}
          </div>
        </div>

        {/* Event Ticker */}
        <aside className="w-72 shrink-0 border-l border-slate-800 bg-slate-900/50 p-4 flex flex-col">
          <h3 className="text-xs font-semibold text-slate-400 uppercase tracking-wider mb-3">📻 Event Log</h3>
          <div className="flex-1 overflow-y-auto space-y-1 text-xs font-mono">
            {eventLog.length === 0 && (
              <p className="text-slate-600 italic">No events yet. Start a storm to see live updates.</p>
            )}
            {eventLog.map((evt, i) => (
              <div key={i} className={`py-1 px-2 rounded ${evt.includes('⚠️') ? 'text-yellow-400 bg-yellow-900/20' :
                  evt.includes('damage') || evt.includes('fail') ? 'text-red-400 bg-red-900/20' :
                  evt.includes('✅') ? 'text-green-400 bg-green-900/20' :
                  evt.includes('🛡️') ? 'text-blue-400 bg-blue-900/20' :
                  'text-slate-400'
                }`}>
                {evt}
              </div>
            ))}
          </div>
          {error && (
            <div className="mt-3 p-3 bg-red-900/30 border border-red-800 rounded-lg text-xs text-red-400">{error}</div>
          )}
        </aside>
      </div>
    </div>
  );
}