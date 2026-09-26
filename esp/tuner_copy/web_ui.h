#ifndef WEB_UI_H
#define WEB_UI_H

#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>MicroMouse Motor & Encoder Tuner</title>
  <style>
    :root {
      --bg: #0b0f17;
      --card-bg: rgba(22, 27, 34, 0.85);
      --card-border: #30363d;
      --text: #e6edf3;
      --text-muted: #8b949e;
      --accent-cyan: #00d2ff;
      --accent-blue: #3a7bd5;
      --accent-green: #10b981;
      --accent-yellow: #f59e0b;
      --accent-red: #ef4444;
      --accent-purple: #8b5cf6;
      --radius: 12px;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; }
    body { background-color: var(--bg); color: var(--text); padding-bottom: 60px; }
    
    /* Top Sticky Header */
    header {
      position: sticky; top: 0; z-index: 100;
      background: rgba(11, 15, 23, 0.95);
      backdrop-filter: blur(10px);
      border-bottom: 1px solid var(--card-border);
      padding: 12px 18px;
      display: flex; justify-content: space-between; align-items: center;
    }
    .header-title { display: flex; align-items: center; gap: 10px; font-weight: 700; font-size: 1.1rem; color: #fff; }
    .badge {
      display: inline-flex; align-items: center; gap: 6px;
      padding: 4px 10px; border-radius: 20px; font-size: 0.75rem; font-weight: 600;
      background: rgba(16, 185, 129, 0.15); color: var(--accent-green); border: 1px solid rgba(16, 185, 129, 0.3);
    }
    .badge.offline { background: rgba(239, 68, 68, 0.15); color: var(--accent-red); border-color: rgba(239, 68, 68, 0.3); }
    .pulse-dot { width: 8px; height: 8px; border-radius: 50%; background: currentColor; }
    
    .quick-bar { display: flex; gap: 8px; }
    .btn-estop {
      background: linear-gradient(135deg, #ef4444, #b91c1c);
      color: #fff; font-weight: 800; border: none; border-radius: 8px;
      padding: 8px 16px; cursor: pointer; display: flex; align-items: center; gap: 6px;
      box-shadow: 0 0 15px rgba(239, 68, 68, 0.4);
    }
    .btn-estop:active { transform: scale(0.96); }

    .container { max-width: 900px; margin: 20px auto; padding: 0 16px; display: flex; flex-direction: column; gap: 20px; }

    /* Cards */
    .card {
      background: var(--card-bg);
      border: 1px solid var(--card-border);
      border-radius: var(--radius);
      padding: 18px 20px;
      box-shadow: 0 4px 20px rgba(0,0,0,0.3);
    }
    .card-title {
      font-size: 1.05rem; font-weight: 700; color: #fff; margin-bottom: 14px;
      display: flex; justify-content: space-between; align-items: center;
      border-bottom: 1px solid rgba(255,255,255,0.06); padding-bottom: 8px;
    }

    /* Wheel Sync & Balance Meter */
    .balance-meter-box { margin: 15px 0 20px 0; text-align: center; }
    .balance-track {
      height: 14px; background: #1c2128; border-radius: 7px;
      position: relative; overflow: hidden; border: 1px solid #30363d; margin: 8px 0;
    }
    .balance-center-line {
      position: absolute; left: 50%; top: 0; bottom: 0; width: 2px;
      background: #8b949e; transform: translateX(-50%); z-index: 2;
    }
    .balance-fill {
      position: absolute; top: 0; bottom: 0;
      background: var(--accent-green); transition: all 0.15s ease-out;
    }
    .balance-status-text { font-size: 0.88rem; font-weight: 600; color: var(--text-muted); }

    .wheel-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 14px; }
    .wheel-card {
      background: #11151c; border: 1px solid #21262d; border-radius: 10px; padding: 14px; text-align: center;
    }
    .wheel-label { font-size: 0.8rem; font-weight: 700; text-transform: uppercase; color: var(--text-muted); letter-spacing: 0.5px; }
    .wheel-ticks { font-size: 1.8rem; font-weight: 800; color: var(--accent-cyan); margin: 6px 0 2px 0; }
    .wheel-meta { font-size: 0.85rem; color: var(--text-muted); display: flex; justify-content: space-around; margin-top: 8px; border-top: 1px dashed #21262d; padding-top: 8px; }
    .wheel-meta span strong { color: #fff; }

    /* Motion Controls */
    .dist-input-row { display: flex; gap: 12px; align-items: center; margin-bottom: 12px; }
    .input-group { flex: 1; display: flex; flex-direction: column; gap: 6px; }
    .input-group label { font-size: 0.8rem; font-weight: 600; color: var(--text-muted); }
    .custom-input {
      background: #0b0f17; border: 1px solid var(--card-border); color: #fff;
      padding: 10px 14px; border-radius: 8px; font-size: 1.1rem; font-weight: 700; width: 100%;
    }
    .custom-input:focus { outline: none; border-color: var(--accent-cyan); }
    .presets-row { display: flex; flex-wrap: wrap; gap: 6px; margin-bottom: 18px; }
    .chip {
      background: #1c2128; border: 1px solid var(--card-border); color: var(--text-muted);
      padding: 6px 12px; border-radius: 16px; font-size: 0.8rem; font-weight: 600; cursor: pointer;
    }
    .chip:hover, .chip:active { background: #262c36; color: #fff; border-color: var(--accent-cyan); }

    .motion-btn-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 12px; margin-top: 12px; }
    .btn-move {
      border: none; border-radius: 10px; padding: 14px; font-size: 1.05rem; font-weight: 700;
      cursor: pointer; display: flex; justify-content: center; align-items: center; gap: 8px;
      transition: all 0.15s ease;
    }
    .btn-move-fwd { background: linear-gradient(135deg, #10b981, #059669); color: #fff; box-shadow: 0 4px 15px rgba(16,185,129,0.25); }
    .btn-move-rev { background: linear-gradient(135deg, #3b82f6, #1d4ed8); color: #fff; box-shadow: 0 4px 15px rgba(59,130,246,0.25); }
    .btn-move:active { transform: scale(0.97); }

    .progress-bar-wrap { margin-top: 15px; }
    .progress-bar-track { height: 8px; background: #1c2128; border-radius: 4px; overflow: hidden; }
    .progress-bar-val { height: 100%; width: 0%; background: linear-gradient(90deg, var(--accent-cyan), var(--accent-green)); transition: width 0.1s linear; }

    /* Sliders & Tuning Inputs */
    .tuning-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 16px; }
    @media(max-width: 600px) { .tuning-grid, .wheel-grid { grid-template-columns: 1fr; } }

    .param-item { display: flex; flex-direction: column; gap: 6px; margin-bottom: 12px; }
    .param-header { display: flex; justify-content: space-between; font-size: 0.85rem; font-weight: 600; color: var(--text-muted); }
    .param-val { color: var(--accent-cyan); font-weight: 700; }
    .param-control-row { display: flex; gap: 10px; align-items: center; }
    .param-slider { flex: 1; -webkit-appearance: none; height: 6px; border-radius: 3px; background: #21262d; outline: none; }
    .param-slider::-webkit-slider-thumb {
      -webkit-appearance: none; width: 18px; height: 18px; border-radius: 50%;
      background: var(--accent-cyan); cursor: pointer; box-shadow: 0 0 8px rgba(0,210,255,0.6);
    }
    .param-num {
      width: 70px; background: #0b0f17; border: 1px solid var(--card-border); color: #fff;
      padding: 6px; border-radius: 6px; font-size: 0.9rem; text-align: center; font-weight: 600;
    }

    /* Diagnostics & Test Buttons */
    .diag-box {
      background: rgba(139, 92, 246, 0.08); border: 1px solid rgba(139, 92, 246, 0.3);
      border-radius: 10px; padding: 14px; margin-bottom: 15px;
    }
    .diag-result {
      margin-top: 10px; padding: 10px; border-radius: 8px; background: #11151c;
      font-size: 0.85rem; color: #e6edf3; display: none; line-height: 1.5;
    }
    .btn-diag {
      background: #21262d; border: 1px solid #30363d; color: #fff; border-radius: 8px;
      padding: 10px 14px; font-weight: 600; cursor: pointer; font-size: 0.88rem; transition: background 0.15s;
    }
    .btn-diag:hover { background: #30363d; }
    .btn-diag-primary { background: linear-gradient(135deg, #8b5cf6, #6d28d9); border: none; }
    .btn-diag-primary:hover { background: #7c3aed; }

    /* Checkbox list */
    .checkbox-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-top: 10px; }
    .check-label { display: flex; align-items: center; gap: 8px; font-size: 0.85rem; color: var(--text-muted); cursor: pointer; }
    .check-label input { width: 16px; height: 16px; accent-color: var(--accent-cyan); }

    /* Action bar */
    .btn-group-row { display: flex; gap: 10px; margin-top: 15px; }
    .btn-secondary { background: #21262d; border: 1px solid #30363d; color: #fff; padding: 10px 16px; border-radius: 8px; font-weight: 600; cursor: pointer; flex: 1; }
    .btn-primary { background: var(--accent-cyan); color: #0b0f17; border: none; padding: 10px 16px; border-radius: 8px; font-weight: 700; cursor: pointer; flex: 1; }
    .btn-secondary:hover { background: #30363d; }
    .btn-primary:hover { filter: brightness(1.1); }
  </style>
</head>
<body>

  <!-- Top Bar -->
  <header>
    <div class="header-title">
      <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 2v4M12 18v4M4.93 4.93l2.83 2.83M16.24 16.24l2.83 2.83M2 12h4M18 12h4M4.93 19.07l2.83-2.83M16.24 7.76l2.83-2.83"/></svg>
      <span>MM TUNER</span>
      <div id="connBadge" class="badge"><div class="pulse-dot"></div><span>CONNECTED</span></div>
    </div>
    <div class="quick-bar">
      <button class="btn-secondary" style="padding:6px 12px; font-size:0.8rem;" onclick="resetEncoders()">Zero Enc</button>
      <button class="btn-estop" onclick="emergencyStop()">
        <svg width="18" height="18" viewBox="0 0 24 24" fill="currentColor"><path d="M12 2a10 10 0 1010 10A10 10 0 0012 2zm1 14h-2v-2h2zm0-4h-2V7h2z"/></svg>
        <span>STOP</span>
      </button>
    </div>
  </header>

  <div class="container">

    <!-- Card 1: Wheel Balance & Live Telemetry -->
    <div class="card">
      <div class="card-title">
        <span>⚡ Live Wheel Synchronizer</span>
        <span id="motionStateBadge" style="font-size:0.8rem; color:var(--accent-cyan); font-weight:700;">IDLE</span>
      </div>

      <!-- Realtime Balance Deviation Meter -->
      <div class="balance-meter-box">
        <div class="balance-status-text" id="balanceStatusText">WHEELS BALANCED (0 TICKS DIFF)</div>
        <div class="balance-track">
          <div class="balance-center-line"></div>
          <div id="balanceFill" class="balance-fill" style="left:50%; width:0%;"></div>
        </div>
        <div style="display:flex; justify-content:space-between; font-size:0.75rem; color:var(--text-muted);">
          <span>◀ Left Lagging (Tight)</span>
          <span>Balanced</span>
          <span>Right Lagging (Tight) ▶</span>
        </div>
      </div>

      <!-- Left / Right Wheel Boxes -->
      <div class="wheel-grid">
        <div class="wheel-card" id="leftWheelCard">
          <div class="wheel-label">Left Wheel</div>
          <div class="wheel-ticks" id="encLeftVal">0</div>
          <div style="font-size:0.8rem; color:var(--accent-cyan);" id="distLeftVal">0.0 mm</div>
          <div class="wheel-meta">
            <span>PWM: <strong id="pwmLVal">0</strong></span>
            <span>Trim: <strong id="trimLDisp">1.00</strong></span>
          </div>
        </div>
        <div class="wheel-card" id="rightWheelCard">
          <div class="wheel-label">Right Wheel</div>
          <div class="wheel-ticks" id="encRightVal">0</div>
          <div style="font-size:0.8rem; color:var(--accent-cyan);" id="distRightVal">0.0 mm</div>
          <div class="wheel-meta">
            <span>PWM: <strong id="pwmRVal">0</strong></span>
            <span>Trim: <strong id="trimRDisp">1.00</strong></span>
          </div>
        </div>
      </div>
    </div>

    <!-- Card 2: Distance Motion Control -->
    <div class="card">
      <div class="card-title">
        <span>🎯 Distance Motion Runner</span>
        <span style="font-size:0.8rem; color:var(--text-muted);">Balanced Closed-Loop Move</span>
      </div>

      <div class="dist-input-row">
        <div class="input-group">
          <label>Target Distance (mm)</label>
          <input type="number" id="targetDistInput" class="custom-input" value="180" step="10" min="10" max="5000">
        </div>
        <div class="input-group">
          <label>Speed (PWM 50-255)</label>
          <input type="number" id="speedPwmInput" class="custom-input" value="120" step="5" min="50" max="255">
        </div>
      </div>

      <!-- Presets -->
      <div class="presets-row">
        <span class="chip" onclick="setDistPreset(90)">90 mm (Half Cell)</span>
        <span class="chip" onclick="setDistPreset(180)">180 mm (1 Cell)</span>
        <span class="chip" onclick="setDistPreset(360)">360 mm (2 Cells)</span>
        <span class="chip" onclick="setDistPreset(540)">540 mm (3 Cells)</span>
        <span class="chip" onclick="setDistPreset(1000)">1000 mm (1 Meter)</span>
      </div>

      <!-- Action Buttons -->
      <div class="motion-btn-grid">
        <button class="btn-move btn-move-fwd" onclick="startMove('fwd')">
          <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><path d="M12 19V5M5 12l7-7 7 7"/></svg>
          <span>MOVE FORWARD</span>
        </button>
        <button class="btn-move btn-move-rev" onclick="startMove('rev')">
          <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><path d="M12 5v14M19 12l-7 7-7-7"/></svg>
          <span>MOVE BACKWARD</span>
        </button>
      </div>

      <!-- Realtime Move Progress -->
      <div class="progress-bar-wrap">
        <div style="display:flex; justify-content:space-between; font-size:0.75rem; color:var(--text-muted); margin-bottom:4px;">
          <span>Progress: <strong id="progressDistText" style="color:#fff;">0.0 mm</strong></span>
          <span id="progressPctText">0%</span>
        </div>
        <div class="progress-bar-track">
          <div id="progressBarVal" class="progress-bar-val"></div>
        </div>
      </div>
    </div>

    <!-- Card 3: Motor Torque & Friction Balancing (Tackling Stiff Motors) -->
    <div class="card">
      <div class="card-title">
        <span>⚖️ Torque & Friction Compensation</span>
        <span style="font-size:0.75rem; color:var(--accent-yellow);">Fixes Stiff/Tight Motors</span>
      </div>
      <p style="font-size:0.82rem; color:var(--text-muted); margin-bottom:15px; line-height:1.4;">
        If one motor is tighter or stiffer, increase its <strong>Trim</strong> (e.g. 1.10 - 1.20) or increase <strong>Min PWM</strong> so it breaks friction instantly without lagging.
      </p>

      <div class="tuning-grid">
        <!-- Left Motor Trim -->
        <div class="param-item">
          <div class="param-header">
            <span>Left Motor Trim Multiplier</span>
            <span class="param-val" id="trimLVal">1.00</span>
          </div>
          <div class="param-control-row">
            <input type="range" class="param-slider" id="sliderTrimL" min="0.60" max="1.50" step="0.01" value="1.00" oninput="syncParam('TrimL', this.value)">
            <input type="number" class="param-num" id="numTrimL" step="0.01" value="1.00" onchange="syncParam('TrimL', this.value)">
          </div>
        </div>

        <!-- Right Motor Trim -->
        <div class="param-item">
          <div class="param-header">
            <span>Right Motor Trim Multiplier</span>
            <span class="param-val" id="trimRVal">1.00</span>
          </div>
          <div class="param-control-row">
            <input type="range" class="param-slider" id="sliderTrimR" min="0.60" max="1.50" step="0.01" value="1.00" oninput="syncParam('TrimR', this.value)">
            <input type="number" class="param-num" id="numTrimR" step="0.01" value="1.00" onchange="syncParam('TrimR', this.value)">
          </div>
        </div>

        <!-- Min PWM Deadband -->
        <div class="param-item">
          <div class="param-header">
            <span>Min PWM Deadband (Friction Floor)</span>
            <span class="param-val" id="minPwmVal">45</span>
          </div>
          <div class="param-control-row">
            <input type="range" class="param-slider" id="sliderMinPwm" min="15" max="100" step="1" value="45" oninput="syncParam('MinPwm', this.value)">
            <input type="number" class="param-num" id="numMinPwm" step="1" value="45" onchange="syncParam('MinPwm', this.value)">
          </div>
        </div>

        <!-- Ticks Per MM -->
        <div class="param-item">
          <div class="param-header">
            <span>Calibration: Ticks per MM</span>
            <span class="param-val" id="ticksMmVal">8.45</span>
          </div>
          <div class="param-control-row">
            <input type="range" class="param-slider" id="sliderTicksMm" min="3.00" max="25.00" step="0.05" value="8.45" oninput="syncParam('TicksMm', this.value)">
            <input type="number" class="param-num" id="numTicksMm" step="0.05" value="8.45" onchange="syncParam('TicksMm', this.value)">
          </div>
        </div>
      </div>

      <div class="card-title" style="margin-top:15px; font-size:0.95rem;">
        <span>Encoder Straight Balance PID</span>
      </div>

      <div class="tuning-grid">
        <!-- Kp Bal -->
        <div class="param-item">
          <div class="param-header">
            <span>Kp (Proportional Sync)</span>
            <span class="param-val" id="kpBalVal">0.45</span>
          </div>
          <div class="param-control-row">
            <input type="range" class="param-slider" id="sliderKpBal" min="0.00" max="2.00" step="0.02" value="0.45" oninput="syncParam('KpBal', this.value)">
            <input type="number" class="param-num" id="numKpBal" step="0.02" value="0.45" onchange="syncParam('KpBal', this.value)">
          </div>
        </div>

        <!-- Ki Bal -->
        <div class="param-item">
          <div class="param-header">
            <span>Ki (Integral - Kills Stiff Motor Offset!)</span>
            <span class="param-val" id="kiBalVal">0.03</span>
          </div>
          <div class="param-control-row">
            <input type="range" class="param-slider" id="sliderKiBal" min="0.00" max="0.20" step="0.005" value="0.03" oninput="syncParam('KiBal', this.value)">
            <input type="number" class="param-num" id="numKiBal" step="0.005" value="0.03" onchange="syncParam('KiBal', this.value)">
          </div>
        </div>

        <!-- Kd Bal -->
        <div class="param-item">
          <div class="param-header">
            <span>Kd (Derivative Damping)</span>
            <span class="param-val" id="kdBalVal">0.15</span>
          </div>
          <div class="param-control-row">
            <input type="range" class="param-slider" id="sliderKdBal" min="0.00" max="1.00" step="0.02" value="0.15" oninput="syncParam('KdBal', this.value)">
            <input type="number" class="param-num" id="numKdBal" step="0.02" value="0.15" onchange="syncParam('KdBal', this.value)">
          </div>
        </div>
      </div>

      <!-- Save & Apply Bar -->
      <div class="btn-group-row">
        <button class="btn-secondary" onclick="uploadConfig(false)">Apply to RAM</button>
        <button class="btn-primary" onclick="uploadConfig(true)">💾 Save to Flash (NVS)</button>
      </div>
    </div>

    <!-- Card 4: Open-Loop Torque & Stiction Diagnostics -->
    <div class="card">
      <div class="card-title">
        <span>🔬 Torque Difference & Motor Diagnostic</span>
      </div>

      <div class="diag-box">
        <div style="font-weight:700; font-size:0.92rem; color:#fff; margin-bottom:6px;">
          1. Automated Open-Loop Friction Measurement (No PID)
        </div>
        <p style="font-size:0.8rem; color:var(--text-muted); margin-bottom:12px;">
          Runs both motors forward at equal PWM for 2 seconds with PID disabled. It reveals the exact raw gearbox friction difference and automatically suggests trim multipliers!
        </p>
        <div style="display:flex; gap:10px; flex-wrap:wrap;">
          <button class="btn-diag btn-diag-primary" onclick="runOpenLoopTest()">Run 2-Second Open Loop Test</button>
        </div>
        <div id="openLoopResult" class="diag-result"></div>
      </div>

      <!-- Solo Motor Tests -->
      <div style="display:grid; grid-template-columns: 1fr 1fr; gap:10px;">
        <button class="btn-diag" onclick="runSoloTest('L')">Test Left Motor (1 sec)</button>
        <button class="btn-diag" onclick="runSoloTest('R')">Test Right Motor (1 sec)</button>
      </div>
    </div>

    <!-- Card 5: Distance Ruler Calibration -->
    <div class="card">
      <div class="card-title">
        <span>📏 Actual Distance Ruler Calibration</span>
      </div>
      <p style="font-size:0.82rem; color:var(--text-muted); margin-bottom:12px;">
        After running a test move (e.g. commanded 200 mm), measure the real distance traveled with a ruler and enter it below:
      </p>
      <div style="display:flex; gap:10px; align-items:center;">
        <input type="number" id="actualMeasuredDist" class="custom-input" placeholder="Actual measured mm (e.g. 196)" style="flex:1;">
        <button class="btn-primary" style="flex:none; padding:10px 18px;" onclick="calibrateTicksFromRuler()">Auto-Calculate Ticks/MM</button>
      </div>
    </div>

    <!-- Card 6: Motor & Encoder Inversion Flags -->
    <div class="card">
      <div class="card-title">
        <span>⚙️ Inversion Flags (Hardware Wiring Fixes)</span>
      </div>
      <p style="font-size:0.8rem; color:var(--text-muted);">
        If a motor drives backwards when forward is commanded, or an encoder counts down instead of up:
      </p>
      <div class="checkbox-grid">
        <label class="check-label"><input type="checkbox" id="chkInvML" onchange="updateInvertFlags()"> Invert Left Motor</label>
        <label class="check-label"><input type="checkbox" id="chkInvMR" onchange="updateInvertFlags()"> Invert Right Motor</label>
        <label class="check-label"><input type="checkbox" id="chkInvEL" onchange="updateInvertFlags()"> Invert Left Encoder</label>
        <label class="check-label"><input type="checkbox" id="chkInvER" onchange="updateInvertFlags()"> Invert Right Encoder</label>
        <label class="check-label" style="color:var(--accent-cyan); font-weight:700;"><input type="checkbox" id="chkSwapM" onchange="updateInvertFlags()"> 🔄 Swap Motors (L ⟷ R)</label>
        <label class="check-label" style="color:var(--accent-cyan); font-weight:700;"><input type="checkbox" id="chkSwapE" onchange="updateInvertFlags()"> 🔄 Swap Encoders (L ⟷ R)</label>
      </div>
    </div>

  </div>

  <script>
    let pollInterval = null;
    let isMoving = false;
    let startDistMm = 0;
    let targetDistMm = 180;
    let suggestedTrimL = 1.0;
    let suggestedTrimR = 1.0;

    // Set preset distance
    function setDistPreset(mm) {
      document.getElementById('targetDistInput').value = mm;
    }

    // Sync input range & numeric field
    function syncParam(name, val) {
      val = parseFloat(val);
      if (isNaN(val)) return;
      document.getElementById('slider' + name).value = val;
      document.getElementById('num' + name).value = val;
      document.getElementById(name.charAt(0).toLowerCase() + name.slice(1) + 'Val').innerText = val;
    }

    // Start Forward or Backward Move
    function startMove(dir) {
      const dist = parseFloat(document.getElementById('targetDistInput').value) || 180;
      const speed = parseInt(document.getElementById('speedPwmInput').value) || 120;
      targetDistMm = dist;
      
      fetch(`/move?dir=${dir}&dist=${dist}&speed=${speed}`)
        .then(res => res.json())
        .then(data => {
          console.log("Move started:", data);
        })
        .catch(err => console.error("Error starting move:", err));
    }

    // Emergency Stop
    function emergencyStop() {
      fetch('/stop')
        .then(res => res.json())
        .then(data => {
          console.log("Emergency stop sent:", data);
        });
    }

    // Zero Encoders
    function resetEncoders() {
      fetch('/reset_enc')
        .then(res => res.json())
        .then(data => {
          console.log("Encoders reset:", data);
        });
    }

    // Solo Motor Test
    function runSoloTest(motor) {
      const speed = parseInt(document.getElementById('speedPwmInput').value) || 120;
      fetch(`/test_solo?motor=${motor}&speed=${speed}&dur=1000`)
        .then(res => res.json())
        .then(data => alert(`Solo test started on ${motor} motor for 1.0 second.`));
    }

    // Open Loop Raw Friction Test
    function runOpenLoopTest() {
      const resBox = document.getElementById('openLoopResult');
      resBox.style.display = 'block';
      resBox.innerHTML = `<em>Testing... Spinning both motors open-loop at PWM 100 for 2 seconds. Keep wheels free or on flat surface!</em>`;
      
      fetch('/open_loop?speed=100&dur=2000')
        .then(res => res.json())
        .then(data => {
          setTimeout(pollOpenLoopResult, 2200);
        });
    }

    function pollOpenLoopResult() {
      fetch('/open_loop_result')
        .then(res => res.json())
        .then(res => {
          const resBox = document.getElementById('openLoopResult');
          if (!res.completed) {
            setTimeout(pollOpenLoopResult, 500);
            return;
          }
          
          let lTicks = res.ticks_l;
          let rTicks = res.ticks_r;
          let diff = lTicks - rTicks;
          let html = `<strong>Results of 2-second Raw Spin:</strong><br>`;
          html += `Left Wheel: <strong>${lTicks}</strong> ticks | Right Wheel: <strong>${rTicks}</strong> ticks (Diff: ${diff})<br>`;
          
          if (Math.abs(diff) < 25) {
            html += `<span style="color:var(--accent-green); font-weight:700;">✅ Excellent balance! Both motors have nearly identical friction.</span>`;
          } else if (lTicks > rTicks) {
            let ratio = (lTicks / Math.max(1, rTicks)).toFixed(3);
            let pct = (((lTicks - rTicks) / lTicks) * 100).toFixed(1);
            suggestedTrimL = 1.00;
            suggestedTrimR = parseFloat(ratio);
            html += `<span style="color:var(--accent-yellow); font-weight:700;">⚠️ Right motor is ${pct}% STIFFER / TIGHTER than Left!</span><br>`;
            html += `Recommended compensation: Keep Left Trim at 1.00 and set <strong>Right Trim = ${suggestedTrimR.toFixed(2)}</strong>.<br>`;
            html += `<button class="btn-primary" style="margin-top:8px; padding:6px 12px; font-size:0.8rem;" onclick="applySuggestedTrims()">Apply Suggested Trims Now</button>`;
          } else {
            let ratio = (rTicks / Math.max(1, lTicks)).toFixed(3);
            let pct = (((rTicks - lTicks) / rTicks) * 100).toFixed(1);
            suggestedTrimR = 1.00;
            suggestedTrimL = parseFloat(ratio);
            html += `<span style="color:var(--accent-yellow); font-weight:700;">⚠️ Left motor is ${pct}% STIFFER / TIGHTER than Right!</span><br>`;
            html += `Recommended compensation: Keep Right Trim at 1.00 and set <strong>Left Trim = ${suggestedTrimL.toFixed(2)}</strong>.<br>`;
            html += `<button class="btn-primary" style="margin-top:8px; padding:6px 12px; font-size:0.8rem;" onclick="applySuggestedTrims()">Apply Suggested Trims Now</button>`;
          }
          resBox.innerHTML = html;
        });
    }

    function applySuggestedTrims() {
      syncParam('TrimL', suggestedTrimL);
      syncParam('TrimR', suggestedTrimR);
      uploadConfig(true);
      alert(`Applied Trims: Left = ${suggestedTrimL.toFixed(2)}, Right = ${suggestedTrimR.toFixed(2)} and saved to Flash!`);
    }

    // Ruler calibration
    function calibrateTicksFromRuler() {
      const actualMm = parseFloat(document.getElementById('actualMeasuredDist').value);
      if (!actualMm || actualMm <= 0) {
        alert("Please enter a valid measured distance in mm.");
        return;
      }
      fetch(`/calibrate_ruler?actual_mm=${actualMm}`)
        .then(res => res.json())
        .then(data => {
          alert(`New Ticks/MM calculated: ${data.ticks_per_mm.toFixed(3)}! Saved to Flash.`);
          syncParam('TicksMm', data.ticks_per_mm);
        });
    }

    // Inversion checkboxes
    function updateInvertFlags() {
      let flags = 0;
      if (document.getElementById('chkInvML').checked) flags |= 0x01;
      if (document.getElementById('chkInvMR').checked) flags |= 0x02;
      if (document.getElementById('chkInvEL').checked) flags |= 0x04;
      if (document.getElementById('chkInvER').checked) flags |= 0x08;
      if (document.getElementById('chkSwapM').checked) flags |= 0x10;
      if (document.getElementById('chkSwapE').checked) flags |= 0x20;
      
      fetch(`/set_inverts?flags=${flags}`)
        .then(res => res.json())
        .then(data => console.log("Invert flags updated:", flags));
    }

    // Upload config to robot
    function uploadConfig(saveFlash) {
      const params = new URLSearchParams({
        trim_l: document.getElementById('numTrimL').value,
        trim_r: document.getElementById('numTrimR').value,
        min_pwm: document.getElementById('numMinPwm').value,
        ticks_mm: document.getElementById('numTicksMm').value,
        kp_bal: document.getElementById('numKpBal').value,
        ki_bal: document.getElementById('numKiBal').value,
        kd_bal: document.getElementById('numKdBal').value,
        save: saveFlash ? "1" : "0"
      });
      
      fetch(`/set_config?${params.toString()}`)
        .then(res => res.json())
        .then(data => {
          if (saveFlash) alert("Parameters successfully saved to ESP32 Flash memory!");
        });
    }

    // Realtime Status Polling Loop (10 Hz)
    function pollStatus() {
      fetch('/status')
        .then(res => res.json())
        .then(data => {
          document.getElementById('connBadge').classList.remove('offline');
          document.getElementById('connBadge').querySelector('span').innerText = 'CONNECTED';
          
          // Update Encoders & Distance
          document.getElementById('encLeftVal').innerText = data.enc_l;
          document.getElementById('encRightVal').innerText = data.enc_r;
          document.getElementById('distLeftVal').innerText = (data.enc_l / data.ticks_mm).toFixed(1) + ' mm';
          document.getElementById('distRightVal').innerText = (data.enc_r / data.ticks_mm).toFixed(1) + ' mm';
          document.getElementById('pwmLVal').innerText = data.pwm_l;
          document.getElementById('pwmRVal').innerText = data.pwm_r;
          document.getElementById('trimLDisp').innerText = data.trim_l.toFixed(2);
          document.getElementById('trimRDisp').innerText = data.trim_r.toFixed(2);
          document.getElementById('motionStateBadge').innerText = data.state;
          
          // Update Balance Meter
          const diff = data.enc_l - data.enc_r;
          const statusText = document.getElementById('balanceStatusText');
          const balanceFill = document.getElementById('balanceFill');
          
          if (Math.abs(diff) <= 2) {
            statusText.innerText = `PERFECT BALANCE (0-2 TICKS DIFF)`;
            statusText.style.color = "var(--accent-green)";
            balanceFill.style.background = "var(--accent-green)";
            balanceFill.style.left = "50%";
            balanceFill.style.width = "0%";
          } else if (diff > 0) {
            // Left has more ticks -> Right is lagging (tight)
            statusText.innerText = `RIGHT WHEEL LAGGING (TIGHT) BY ${diff} TICKS [Steer: ${data.steer.toFixed(1)}]`;
            statusText.style.color = "var(--accent-yellow)";
            balanceFill.style.background = "var(--accent-yellow)";
            balanceFill.style.left = "50%";
            let pct = Math.min(50, (diff / 30) * 50);
            balanceFill.style.width = `${pct}%`;
          } else {
            // Right has more ticks -> Left is lagging (tight)
            let absDiff = Math.abs(diff);
            statusText.innerText = `LEFT WHEEL LAGGING (TIGHT) BY ${absDiff} TICKS [Steer: ${data.steer.toFixed(1)}]`;
            statusText.style.color = "var(--accent-yellow)";
            balanceFill.style.background = "var(--accent-yellow)";
            let pct = Math.min(50, (absDiff / 30) * 50);
            balanceFill.style.left = `${50 - pct}%`;
            balanceFill.style.width = `${pct}%`;
          }

          // Progress bar
          if (data.target_dist > 0) {
            let progress = (data.dist_traveled / data.target_dist) * 100;
            progress = Math.min(100, Math.max(0, progress));
            document.getElementById('progressBarVal').style.width = `${progress}%`;
            document.getElementById('progressDistText').innerText = `${data.dist_traveled.toFixed(1)} / ${data.target_dist.toFixed(1)} mm`;
            document.getElementById('progressPctText').innerText = `${Math.round(progress)}%`;
          } else {
            document.getElementById('progressBarVal').style.width = `0%`;
            document.getElementById('progressDistText').innerText = `0.0 mm`;
            document.getElementById('progressPctText').innerText = `0%`;
          }
        })
        .catch(err => {
          document.getElementById('connBadge').classList.add('offline');
          document.getElementById('connBadge').querySelector('span').innerText = 'OFFLINE';
        });
    }

    // Initial config load from robot
    function loadInitialConfig() {
      fetch('/status')
        .then(res => res.json())
        .then(cfg => {
          syncParam('TrimL', cfg.trim_l);
          syncParam('TrimR', cfg.trim_r);
          syncParam('MinPwm', cfg.min_pwm);
          syncParam('TicksMm', cfg.ticks_mm);
          syncParam('KpBal', cfg.kp_bal);
          syncParam('KiBal', cfg.ki_bal);
          syncParam('KdBal', cfg.kd_bal);

          document.getElementById('chkInvML').checked = !!(cfg.inv_flags & 0x01);
          document.getElementById('chkInvMR').checked = !!(cfg.inv_flags & 0x02);
          document.getElementById('chkInvEL').checked = !!(cfg.inv_flags & 0x04);
          document.getElementById('chkInvER').checked = !!(cfg.inv_flags & 0x08);
          document.getElementById('chkSwapM').checked = !!(cfg.inv_flags & 0x10);
          document.getElementById('chkSwapE').checked = !!(cfg.inv_flags & 0x20);
        });
    }

    window.onload = function() {
      loadInitialConfig();
      pollInterval = setInterval(pollStatus, 150);
    };
  </script>
</body>
</html>
)rawliteral";

#endif
