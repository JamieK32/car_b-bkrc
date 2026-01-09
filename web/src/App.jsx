import React, { useState, useEffect, useMemo } from 'react';
import { 
  Box, 
  Map as MapIcon, 
  TrafficCone, 
  Lamp, 
  Volume2, 
  Clock, 
  Fence, 
  Flag, 
  Navigation, 
  Radio, 
  Cpu, 
  Car,
  Trash2,
  RefreshCw,
  Move,
  Settings,
  Info,
  Code,
  Copy
} from 'lucide-react';

// 更新：行和列全部采用倒序排列
const ROW_LABELS = ['G', 'F', 'E', 'D', 'C', 'B', 'A']; 
const COL_LABELS = [7, 6, 5, 4, 3, 2, 1];
const CELL_SIZE = 80;

const MARKER_TYPES = [
  { id: '3d_display', name: '立体显示', icon: <Box size={18} />, color: 'text-blue-500' },
  { id: 'garage_1', name: '车库1', icon: <Car size={18} />, color: 'text-indigo-600' },
  { id: 'garage_2', name: '车库2', icon: <Car size={18} />, color: 'text-indigo-400' },
  { id: 'traffic_light_a', name: '交通灯 A', icon: <TrafficCone size={18} />, color: 'text-red-500' },
  { id: 'traffic_light_b', name: '交通灯 B', icon: <TrafficCone size={18} />, color: 'text-green-500' },
  { id: 'traffic_light_c', name: '交通灯 C', icon: <TrafficCone size={18} />, color: 'text-yellow-500' },
  { id: 'road_gate', name: '道闸', icon: <Fence size={18} />, color: 'text-orange-500' },
  { id: 'smart_lamp', name: '智能路灯', icon: <Lamp size={18} />, color: 'text-yellow-500' },
  { id: 'static_sign_a', name: '静态标志 A', icon: <Flag size={18} />, color: 'text-slate-600' },
  { id: 'static_sign_b', name: '静态标志物 B', icon: <Flag size={18} />, color: 'text-slate-400' },
  { id: 'beacon', name: '烽火台', icon: <Radio size={18} />, color: 'text-red-700' },
  { id: 'wireless_radio', name: '无线电', icon: <Radio size={18} />, color: 'text-sky-500' },
  { id: 'etc', name: 'ETC', icon: <Cpu size={18} />, color: 'text-teal-600' },
  { id: 'voice', name: '语音播报', icon: <Volume2 size={18} />, color: 'text-purple-500' },
  { id: 'tft_b', name: 'TFT B', icon: <Box size={18} />, color: 'text-cyan-500' },
  { id: 'terrain', name: '特殊地形', icon: <MapIcon size={18} />, color: 'text-emerald-700' },
  { id: 'led_timer', name: 'LED 标志物', icon: <Clock size={18} />, color: 'text-red-600' },
];

const App = () => {
  const [markers, setMarkers] = useState([]);
  const [pathInput, setPathInput] = useState('B7->B6->D6->D4->F4->F2');
  const [parsedPath, setParsedPath] = useState([]);
  const [draggedItem, setDraggedItem] = useState(null);
  const [movingIdx, setMovingIdx] = useState(null);
  const [turnMode, setTurnMode] = useState('relative'); // relative: Car_Turn, absolute: Car_Turn_Gryo

  const wrap180 = (deg) => {
    let a = deg;
    while (a > 180) a -= 360;
    while (a < -180) a += 360;
    return a;
  };

  // 解析路径
  useEffect(() => {
    const segments = pathInput.toUpperCase().split(/[\s,，]+|->/).filter(p => p.trim() !== "");
    const coords = segments.filter(p => {
      const rowChar = p[0];
      const colNum = parseInt(p.substring(1));
      return ROW_LABELS.includes(rowChar) && COL_LABELS.includes(colNum);
    }).map(p => ({ 
      row: p[0], 
      col: parseInt(p.substring(1)),
      raw: p 
    }));
    setParsedPath(coords);
  }, [pathInput]);

  // 根据标签获取像素位置 (用于地图渲染)
  const getPos = (rowLabel, colLabel) => ({
    x: COL_LABELS.indexOf(colLabel) * CELL_SIZE + CELL_SIZE / 2,
    y: ROW_LABELS.indexOf(rowLabel) * CELL_SIZE + CELL_SIZE / 2
  });

  // 自动生成 C 语言代码
  const generatedCCode = useMemo(() => {
    if (parsedPath.length < 2) return "// 请输入路径坐标 (例如: B7->B6)";
    
    let code = "void Car_RunPath(void) {\n";
    code += "    uint8_t speed_val = 30; // 局部速度变量\n\n";

    // 绝对角模式：假设起点朝向为 0°（归零后），后续转向用绝对 yaw 累积表示
    let absYaw = 0;
    
    for (let i = 0; i < parsedPath.length - 1; i++) {
      const current = parsedPath[i];
      const next = parsedPath[i+1];
      
      code += `    // 路径段: ${current.raw} -> ${next.raw}\n`;
      
      if (i === 0) {
        code += `    LSM6DSV16X_RezeroYaw_Fresh(200); // 陀螺仪归零\n`;
        code += `    Car_MoveForward(speed_val, 1200); // 发车段特殊指令\n`;
      } else if (i == parsedPath.length - 2) {
        code += `    Car_BackIntoGarage_Gyro(30, 1400, 90); //倒车入库\n`;
      }
      else {
        code += `    Car_TrackToCross(speed_val);\n`;
      }
      
      // 转向逻辑判定
      if (i < parsedPath.length - 2) {
        const nextNext = parsedPath[i+2];
        
        // 向量 v1: current -> next
        const dx1 = COL_LABELS.indexOf(next.col) - COL_LABELS.indexOf(current.col);
        const dy1 = ROW_LABELS.indexOf(next.row) - ROW_LABELS.indexOf(current.row);
        
        // 向量 v2: next -> nextNext
        const dx2 = COL_LABELS.indexOf(nextNext.col) - COL_LABELS.indexOf(next.col);
        const dy2 = ROW_LABELS.indexOf(nextNext.row) - ROW_LABELS.indexOf(next.row);
        
        // 归一化方向
        const v1 = { x: dx1 === 0 ? 0 : dx1 / Math.abs(dx1), y: dy1 === 0 ? 0 : dy1 / Math.abs(dy1) };
        const v2 = { x: dx2 === 0 ? 0 : dx2 / Math.abs(dx2), y: dy2 === 0 ? 0 : dy2 / Math.abs(dy2) };

        // 叉乘计算转向 (y 轴向下)
        const crossProduct = v1.x * v2.y - v1.y * v2.x;
        
        if (turnMode === 'relative') {
          if (crossProduct > 0) {
            code += `    Car_Turn(90);  // 右转 90°\n`;
          } else if (crossProduct < 0) {
            code += `    Car_Turn(-90); // 左转 90°\n`;
          } else {
            const dotProduct = v1.x * v2.x + v1.y * v2.y;
            if (dotProduct < -0.5) {
              code += `    Car_Turn(180); // 掉头\n`;
            }
          }
        } else {
          if (crossProduct > 0) {
            absYaw = wrap180(absYaw + 90);
            code += `    Car_Turn_Gryo(${absYaw});  // 右转到 ${absYaw}°\n`;
          } else if (crossProduct < 0) {
            absYaw = wrap180(absYaw - 90);
            code += `    Car_Turn_Gryo(${absYaw}); // 左转到 ${absYaw}°\n`;
          } else {
            const dotProduct = v1.x * v2.x + v1.y * v2.y;
            if (dotProduct < -0.5) {
              absYaw = wrap180(absYaw + 180);
              code += `    Car_Turn_Gryo(${absYaw}); // 掉头到 ${absYaw}°\n`;
            }
          }
        }
      }
    }
    
    code += "}";
    return code;
  }, [parsedPath, turnMode]);

  const handleDrop = (e, row, col) => {
    e.preventDefault();
    if (draggedItem) {
      setMarkers([...markers, { ...draggedItem, row, col, uid: Date.now() }]);
      setDraggedItem(null);
    } else if (movingIdx !== null) {
      const next = [...markers];
      next[movingIdx] = { ...next[movingIdx], row, col };
      setMarkers(next);
      setMovingIdx(null);
    }
  };

  return (
    <div className="flex h-screen bg-[#1a1c1e] text-slate-200 overflow-hidden font-sans">
      {/* 侧边栏 */}
      <div className="w-80 bg-[#25282c] border-r border-white/5 flex flex-col shadow-2xl z-50">
        <div className="p-6 border-b border-white/5 bg-gradient-to-br from-blue-600/20 to-transparent">
          <div className="flex items-center gap-3">
            <div className="p-2 bg-blue-600 rounded-lg shadow-lg shadow-blue-600/20">
              <Navigation size={20} className="text-white" />
            </div>
            <h1 className="text-lg font-bold tracking-tight">智能赛道控制台</h1>
          </div>
        </div>

        <div className="flex-1 overflow-y-auto custom-scrollbar flex flex-col">
          {/* 库 */}
          <div className="p-4 border-b border-white/5">
            <h2 className="text-[11px] font-bold text-slate-500 mb-4 px-2 flex items-center gap-2 uppercase tracking-widest">
              <Box size={12} /> 标志物库
            </h2>
            <div className="grid grid-cols-2 gap-2">
              {MARKER_TYPES.map((type) => (
                <div
                  key={type.id}
                  draggable
                  onDragStart={() => setDraggedItem(type)}
                  className="group flex flex-col items-center p-2 bg-white/5 border border-white/5 rounded-xl cursor-grab active:cursor-grabbing hover:bg-white/10 hover:border-blue-500/50 transition-all text-center"
                >
                  <div className={`${type.color} mb-1`}>{type.icon}</div>
                  <span className="text-[10px] font-medium text-slate-400">{type.name}</span>
                </div>
              ))}
            </div>
          </div>

          {/* 代码预览 */}
          <div className="flex-1 p-4 bg-black/20">
            <div className="flex items-center justify-between mb-3 px-2">
              <h2 className="text-[11px] font-bold text-blue-400 flex items-center gap-2 uppercase tracking-widest">
                <Code size={12} /> C 语言逻辑生成
              </h2>
              <button onClick={() => {
                const el = document.createElement('textarea');
                el.value = generatedCCode;
                document.body.appendChild(el);
                el.select();
                document.execCommand('copy');
                document.body.removeChild(el);
              }} className="text-slate-500 hover:text-white transition-colors">
                <Copy size={14} />
              </button>
            </div>
            <div className="bg-black/40 rounded-xl p-3 border border-white/5 font-mono text-[10px] text-emerald-400/90 leading-relaxed overflow-x-auto min-h-[250px]">
              <pre>{generatedCCode}</pre>
            </div>
          </div>
        </div>

        <div className="p-6 bg-[#1e2124] border-t border-white/5">
          <label className="text-[11px] font-bold text-slate-500 uppercase tracking-wider block mb-2 text-center underline">
            坐标系：行(G-A) | 列(7-1)
          </label>
          <div className="flex gap-2 mb-3">
            <button
              onClick={() => setTurnMode('relative')}
              className={`flex-1 py-2 rounded-xl text-xs font-bold border transition-all ${
                turnMode === 'relative'
                  ? 'bg-blue-500/20 text-blue-300 border-blue-500/30'
                  : 'bg-white/5 text-slate-400 border-white/10 hover:bg-white/10'
              }`}
            >
              相对角度 (Car_Turn)
            </button>
            <button
              onClick={() => setTurnMode('absolute')}
              className={`flex-1 py-2 rounded-xl text-xs font-bold border transition-all ${
                turnMode === 'absolute'
                  ? 'bg-blue-500/20 text-blue-300 border-blue-500/30'
                  : 'bg-white/5 text-slate-400 border-white/10 hover:bg-white/10'
              }`}
            >
              绝对角度 (Car_Turn_Gryo)
            </button>
          </div>
          <textarea
            value={pathInput}
            onChange={(e) => setPathInput(e.target.value)}
            className="w-full h-20 p-3 bg-black/30 text-blue-400 text-sm font-mono border border-white/5 rounded-xl focus:ring-1 focus:ring-blue-500/50 outline-none resize-none"
            placeholder="G7->F7->F6..."
          />
          <div className="flex gap-2 mt-4">
            <button 
              onClick={() => { setMarkers([]); setPathInput(''); }}
              className="flex-1 py-2 bg-red-500/10 text-red-400 rounded-xl text-xs font-bold border border-red-500/20 hover:bg-red-500/20 transition-all"
            >
              重置地图
            </button>
          </div>
        </div>
      </div>

      {/* 地图舞台 */}
      <div className="flex-1 relative overflow-hidden flex items-center justify-center">
        <div className="absolute inset-0 opacity-10 pointer-events-none" 
             style={{ backgroundImage: 'radial-gradient(#ffffff 1px, transparent 1px)', backgroundSize: '40px 40px' }} />

        <div className="relative bg-white shadow-[0_0_100px_rgba(0,0,0,0.5)] p-12 border-[12px] border-slate-800 rounded-sm overflow-visible">
          
          {/* 轴刻度 */}
          <div className="absolute -left-10 top-12 bottom-12 flex flex-col justify-between py-10 items-end">
            {ROW_LABELS.map(r => <span key={r} className="text-slate-500 font-black text-sm pr-2">{r}</span>)}
          </div>
          <div className="absolute -top-10 left-12 right-12 flex justify-between px-10 items-end">
            {COL_LABELS.map(c => <span key={c} className="text-slate-500 font-black text-sm pb-2">{c}</span>)}
          </div>

          <div className="relative" style={{ width: CELL_SIZE * 7, height: CELL_SIZE * 7 }}>
            {/* 网格辅助 */}
            <div className="absolute inset-0 grid grid-cols-7 grid-rows-7 opacity-5 pointer-events-none">
              {Array.from({length: 49}).map((_, i) => <div key={i} className="border border-black" />)}
            </div>

            {/* 实线赛道 */}
            <svg className="absolute inset-0 z-0 pointer-events-none" width="560" height="560">
              {['B', 'D', 'F'].map(r => {
                const y = ROW_LABELS.indexOf(r) * CELL_SIZE + 40;
                return <line key={`h-${r}`} x1="0" y1={y} x2="560" y2={y} stroke="#111" strokeWidth="12" />;
              })}
              {[2, 4, 6].map(c => {
                const x = COL_LABELS.indexOf(c) * CELL_SIZE + 40;
                return <line key={`v-${c}`} x1={x} y1="0" x2={x} y2="560" stroke="#111" strokeWidth="12" />;
              })}
            </svg>

            {/* 路径引导线 */}
            <svg className="absolute inset-0 z-10 pointer-events-none" width="560" height="560">
              {parsedPath.length > 1 && parsedPath.map((p, i) => {
                if (i === 0) return null;
                const prev = getPos(parsedPath[i-1].row, parsedPath[i-1].col);
                const curr = getPos(p.row, p.col);
                return (
                  <g key={`path-${i}`}>
                    <line x1={prev.x} y1={prev.y} x2={curr.x} y2={curr.y} stroke="#3b82f6" strokeWidth="8" strokeLinecap="round" opacity="0.3" />
                    <line x1={prev.x} y1={prev.y} x2={curr.x} y2={curr.y} stroke="#60a5fa" strokeWidth="2" strokeDasharray="6 4" strokeLinecap="round" />
                    <circle cx={curr.x} cy={curr.y} r="4" fill="#60a5fa" />
                  </g>
                );
              })}
              {parsedPath.length > 0 && (
                <circle 
                  cx={getPos(parsedPath[0].row, parsedPath[0].col).x} 
                  cy={getPos(parsedPath[0].row, parsedPath[0].col).y} 
                  r="6" fill="#10b981" className="animate-pulse" 
                />
              )}
            </svg>

            {/* 标志物放置层 */}
            <div className="absolute inset-0 z-20 grid grid-cols-7 grid-rows-7">
              {ROW_LABELS.map(rowLabel => (
                COL_LABELS.map(colLabel => (
                  <div
                    key={`${rowLabel}${colLabel}`}
                    onDragOver={(e) => e.preventDefault()}
                    onDrop={(e) => handleDrop(e, rowLabel, colLabel)}
                    className="relative group flex items-center justify-center w-20 h-20"
                  >
                    <div className="w-1.5 h-1.5 bg-slate-200 rounded-full opacity-30 group-hover:opacity-100 transition-opacity" />
                    
                    {markers.map((m, idx) => (m.row === rowLabel && m.col === colLabel) ? (
                      <div
                        key={m.uid}
                        draggable
                        onDragStart={() => setMovingIdx(idx)}
                        className="absolute z-30 flex flex-col items-center justify-center transform hover:scale-110 transition-all cursor-move group/marker"
                      >
                        <div className="relative p-2 rounded-lg bg-[#25282c] border border-slate-700 shadow-xl shadow-black/50">
                          <div className={`${m.color}`}>{m.icon}</div>
                        </div>
                        <span className="mt-4 px-2 py-0.5 bg-black/80 text-[9px] font-bold text-white rounded uppercase whitespace-nowrap shadow-lg">
                          {m.name}
                        </span>
                        <button 
                          onClick={(e) => { e.stopPropagation(); setMarkers(markers.filter((_, i) => i !== idx)); }}
                          className="absolute -top-1 -right-1 bg-red-600 text-white rounded-full p-0.5 opacity-0 group-hover/marker:opacity-100 shadow-lg scale-0 group-hover/marker:scale-100 transition-all"
                        >
                          <Trash2 size={8} />
                        </button>
                      </div>
                    ) : null)}
                  </div>
                ))
              ))}
            </div>
          </div>
        </div>

        {/* 底部信息栏 */}
        <div className="absolute bottom-8 left-1/2 -translate-x-1/2 bg-[#25282c] border border-white/10 px-6 py-3 rounded-2xl shadow-2xl flex items-center gap-8 text-[11px] font-bold tracking-widest text-slate-400">
          <div className="flex items-center gap-2">
            <div className="w-2 h-2 rounded-full bg-blue-500 animate-pulse" />
            <span>转向正负已切换：右(90) | 左(-90)</span>
          </div>
          <div className="w-px h-4 bg-white/10" />
          <div className="flex items-center gap-2 text-blue-400">
            <Settings size={14} />
            <span>实时同步 C 语言路径逻辑</span>
          </div>
        </div>
      </div>

      <style dangerouslySetInnerHTML={{ __html: `
        .custom-scrollbar::-webkit-scrollbar { width: 4px; }
        .custom-scrollbar::-webkit-scrollbar-thumb { background: rgba(255,255,255,0.1); border-radius: 10px; }
      `}} />
    </div>
  );
};

export default App;
