import { useEffect, useRef, useState } from 'react';
import * as d3 from 'd3';
import type { GridData } from '../types/grid';

interface Props {
  gridData: GridData;
  onEdgeClick?: (edgeId: number) => void;
  onNodeClick?: (nodeId: number) => void;
  highlightMode?: 'view' | 'cutEdge' | 'spikeDemand';
  showHeatmap?: boolean;
}

// SVG icons for different node types
function getNodeShape(nodeType: string, x: number, y: number, r: number): string {
  switch (nodeType) {
    case 'GENERATOR':
      // Power plant: rectangle with lightning bolt
      return `M${x - r},${y - r * 0.7} h${r * 2} v${r * 1.4} h-${r * 2} z M${x - r * 0.3},${y - r * 0.7} l${r * 0.6},${r * 0.8} l-${r * 0.6},${r * 0.6}`;
    case 'SUBSTATION':
      // Substation: diamond shape
      return `M${x},${y - r} L${x + r},${y} L${x},${y + r} L${x - r},${y} Z`;
    case 'LOAD':
      // Load: circle (different sizes based on criticality)
      return '';
    default:
      return '';
  }
}

function getNodeColor(nodeType: string, tier: string, loadPct: number, showHeatmap: boolean): string {
  if (nodeType === 'GENERATOR') return '#3b82f6';
  if (nodeType === 'SUBSTATION') return '#8b5cf6';

  if (tier === 'CRITICAL') return '#ef4444'; // Hospitals always red
  if (showHeatmap) return d3.interpolateRdYlGn(1 - loadPct);
  return loadPct > 0.9 ? '#ef4444' : loadPct > 0.7 ? '#f59e0b' : loadPct > 0.4 ? '#22c55e' : '#6ee7b7';
}

function getNodeSize(nodeType: string, tier: string): number {
  switch (nodeType) {
    case 'GENERATOR': return 22;
    case 'SUBSTATION': return 15;
    case 'LOAD':
      if (tier === 'CRITICAL') return 14;
      if (tier === 'COMMERCIAL') return 10;
      return 6;
    default: return 8;
  }
}

function getNodeIcon(nodeType: string, tier: string): string {
  if (nodeType === 'GENERATOR') return '🏭';
  if (nodeType === 'SUBSTATION') return '⚡';
  if (tier === 'CRITICAL') return '🏥';
  if (tier === 'COMMERCIAL') return '🏢';
  return '🏠';
}

export default function GridCanvas({ gridData, onEdgeClick, onNodeClick, highlightMode, showHeatmap }: Props) {
  const svgRef = useRef<SVGSVGElement>(null);
  const [hoveredId, setHoveredId] = useState<{ type: 'node' | 'edge'; id: number } | null>(null);
  const [tooltip, setTooltip] = useState<{ x: number; y: number; text: string } | null>(null);

  useEffect(() => {
    if (!gridData || !svgRef.current) return;
    const svg = d3.select(svgRef.current);
    svg.selectAll('*').remove();

    const width = 1200, height = 800;
    svg.attr('viewBox', `0 0 ${width} ${height}`);

    // Background with grid pattern
    svg.append('rect').attr('width', width).attr('height', height).attr('fill', '#0f172a');

    // Draw road-like grid
    for (let x = 50; x < width; x += 100) {
      svg.append('line').attr('x1', x).attr('y1', 0).attr('x2', x).attr('y2', height)
        .attr('stroke', '#1e293b').attr('stroke-width', 0.5).attr('stroke-dasharray', '4,8');
    }
    for (let y = 50; y < height; y += 100) {
      svg.append('line').attr('x1', 0).attr('y1', y).attr('x2', width).attr('y2', y)
        .attr('stroke', '#1e293b').attr('stroke-width', 0.5).attr('stroke-dasharray', '4,8');
    }

    // Draw edges (transmission lines)
    const maxFlow = Math.max(...gridData.edges.map((e) => e.currentFlowMW), 1);

    gridData.edges.forEach((edge) => {
      const from = gridData.nodes.find((n) => n.id === edge.from);
      const to = gridData.nodes.find((n) => n.id === edge.to);
      if (!from || !to) return;

      const pct = edge.capacityMW > 0 ? edge.currentFlowMW / edge.capacityMW : 0;
      const flowRatio = maxFlow > 0 ? edge.currentFlowMW / maxFlow : 0;

      let color: string;
      if (edge.tripped) color = '#374151';
      else if (showHeatmap) color = d3.interpolateRdYlGn(1 - flowRatio);
      else if (pct > 0.9) color = '#ef4444';
      else if (pct > 0.7) color = '#f59e0b';
      else if (pct > 0.4) color = '#fbbf24';
      else color = '#4ade80';

      const isHovered = hoveredId?.type === 'edge' && hoveredId.id === edge.id;

      svg.append('line')
        .attr('x1', from.x).attr('y1', from.y).attr('x2', to.x).attr('y2', to.y)
        .attr('stroke', color)
        .attr('stroke-width', isHovered ? 5 : 1 + pct * 3)
        .attr('stroke-linecap', 'round')
        .attr('opacity', edge.tripped ? 0.15 : 0.7)
        .attr('cursor', highlightMode === 'cutEdge' ? 'crosshair' : 'pointer')
        .style('transition', 'all 0.2s')
        .on('mouseenter', (e) => {
          setHoveredId({ type: 'edge', id: edge.id });
          setTooltip({ x: e.offsetX, y: e.offsetY, text: `Transmission Line ${edge.id}\nFlow: ${edge.currentFlowMW.toFixed(1)}/${edge.capacityMW} MW\n${edge.tripped ? '⚡ TRIPPED' : '✅ Active'}` });
        })
        .on('mouseleave', () => { setHoveredId(null); setTooltip(null); })
        .on('click', () => onEdgeClick?.(edge.id));

      // Animated flow particles
      if (!edge.tripped && edge.currentFlowMW > 0.1) {
        const particle = svg.append('circle').attr('r', 2.5).attr('fill', '#fbbf24').attr('opacity', 0.8);
        particle.append('animateMotion')
          .attr('dur', `${Math.max(0.3, 2 - pct)}s`)
          .attr('repeatCount', 'indefinite')
          .attr('path', `M${from.x},${from.y} L${to.x},${to.y}`);
      }
    });

    // Draw nodes as icons
    gridData.nodes.forEach((node) => {
      const r = getNodeSize(node.type, node.tier);
      const loadPct = node.capacityMW > 0 ? node.currentLoadMW / node.capacityMW : 0;
      const color = getNodeColor(node.type, node.tier, loadPct, showHeatmap || false);
      const isHovered = hoveredId?.type === 'node' && hoveredId.id === node.id;

      const g = svg.append('g').attr('cursor', highlightMode === 'spikeDemand' ? 'crosshair' : 'pointer');

      // Glow
      g.append('circle').attr('cx', node.x).attr('cy', node.y).attr('r', r + 5)
        .attr('fill', color).attr('opacity', isHovered ? 0.5 : 0.2)
        .attr('pointer-events', 'none').style('transition', 'all 0.2s');

      // Main shape
      if (node.type === 'GENERATOR') {
        // Power plant: rectangle
        g.append('rect')
          .attr('x', node.x - r).attr('y', node.y - r * 0.7)
          .attr('width', r * 2).attr('height', r * 1.4)
          .attr('rx', 3).attr('fill', color).attr('stroke', '#0f172a').attr('stroke-width', 2);
      } else if (node.type === 'SUBSTATION') {
        // Substation: diamond
        g.append('polygon')
          .attr('points', `${node.x},${node.y - r} ${node.x + r},${node.y} ${node.x},${node.y + r} ${node.x - r},${node.y}`)
          .attr('fill', color).attr('stroke', '#0f172a').attr('stroke-width', 2);
      } else {
        // Loads: circle
        g.append('circle')
          .attr('cx', node.x).attr('cy', node.y).attr('r', r)
          .attr('fill', color).attr('stroke', '#0f172a').attr('stroke-width', 1.5);
      }

      // Emoji icon
      g.append('text')
        .attr('x', node.x).attr('y', node.y + 1)
        .attr('text-anchor', 'middle').attr('dominant-baseline', 'central')
        .attr('font-size', r > 12 ? '14px' : r > 8 ? '10px' : '7px')
        .attr('pointer-events', 'none')
        .text(getNodeIcon(node.type, node.tier));

      g.on('mouseenter', (e) => {
        setHoveredId({ type: 'node', id: node.id });
        setTooltip({ x: e.offsetX, y: e.offsetY, text: `${node.type} ${node.id} ${getNodeIcon(node.type, node.tier)}\nTier: ${node.tier}\nCap: ${node.capacityMW} MW\nLoad: ${node.currentLoadMW.toFixed(1)} MW` });
      })
      .on('mouseleave', () => { setHoveredId(null); setTooltip(null); })
      .on('click', () => onNodeClick?.(node.id));
    });

    // City label
    svg.append('text').attr('x', width / 2).attr('y', height - 15)
      .attr('text-anchor', 'middle').attr('fill', '#334155').attr('font-size', '12px')
      .text('GRIDPULSE CITY — Power Distribution Network');

  }, [gridData, hoveredId, highlightMode, showHeatmap, onEdgeClick, onNodeClick]);

  return (
    <div className="relative w-full h-full overflow-hidden">
      <svg ref={svgRef} className="w-full h-full" />
      {tooltip && (
        <div className="absolute z-50 pointer-events-none bg-slate-800/95 border border-slate-600 rounded-lg px-3 py-2 text-xs text-white whitespace-pre-line shadow-2xl backdrop-blur-sm"
          style={{ left: Math.min(tooltip.x + 12, 1050), top: Math.max(tooltip.y - 30, 10) }}>
          {tooltip.text}
        </div>
      )}
    </div>
  );
}