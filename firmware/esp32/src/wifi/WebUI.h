// ============================================================
//  WebUI.h
//  HTML control page for the KIRBY robot.
//  Included by WiFiController.cpp and sent as the response
//  to GET /.
//
//  Endpoints used by the interface:
//    GET  /stufe?n=1..3
//    GET  /parameter?turn=&sideTurn=&stop=&turnFactor=&revDuration=
//    GET  /status   → "State:N Turn:N Stop:N Side:N TF:N Rev:N
//                       Speed:N Lvl:N M:N L:N R:N BTM:N"
// ============================================================

#pragma once

static const char HTML_SEITE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>KIRBY</title>
<style>
  *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

  :root {
    --green:  #1D9E75;
    --blue:   #378ADD;
    --red:    #E24B4A;
    --bg:     #111111;
    --card:   #1E1E1E;
    --border: #2A2A2A;
    --text:   #EEEEEE;
    --muted:  #666666;
    --dim:    #444441;
  }

  body {
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
    background: var(--bg);
    color: var(--text);
    display: flex;
    flex-direction: column;
    align-items: center;
    min-height: 100vh;
    padding: 24px 16px;
    gap: 16px;
  }

  h1 {
    font-size: 24px;
    font-weight: 600;
    letter-spacing: 3px;
    color: var(--green);
    margin-bottom: 4px;
  }

  /* ── Status card ── */
  .card {
    background: var(--card);
    border: 1px solid var(--border);
    border-radius: 14px;
    padding: 16px 20px;
    width: 100%;
    max-width: 360px;
  }

  .card-title {
    font-size: 11px;
    font-weight: 600;
    letter-spacing: 1px;
    text-transform: uppercase;
    color: var(--muted);
    margin-bottom: 10px;
  }

  #state-badge {
    display: inline-block;
    padding: 3px 10px;
    border-radius: 20px;
    font-size: 13px;
    font-weight: 600;
    letter-spacing: 0.5px;
    margin-bottom: 10px;
  }
  .badge-init  { background: #2a2a2a; color: var(--muted); }
  .badge-ready { background: #1a3a2a; color: var(--green); }
  .badge-run   { background: #1a3a2a; color: #4fffb0;      }
  .badge-stop  { background: #3a1a1a; color: var(--red);   }

  .sensor-grid {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 8px;
    margin-top: 6px;
  }

  .sensor-item {
    background: var(--bg);
    border-radius: 8px;
    padding: 8px;
    text-align: center;
  }

  .sensor-label {
    font-size: 10px;
    color: var(--muted);
    margin-bottom: 3px;
    text-transform: uppercase;
    letter-spacing: 0.5px;
  }

  .sensor-value {
    font-size: 18px;
    font-weight: 600;
    color: var(--text);
  }

  .sensor-value.warn  { color: #F5A623; }
  .sensor-value.alert { color: var(--red); }
  .sensor-value.ok    { color: var(--green); }

  /* ── Buttons ── */
  .btn {
    width: 100%;
    max-width: 360px;
    padding: 16px;
    border: none;
    border-radius: 12px;
    font-size: 16px;
    font-weight: 600;
    cursor: pointer;
    transition: opacity 0.15s, transform 0.1s;
    letter-spacing: 0.3px;
  }

  .btn:active { opacity: 0.75; transform: scale(0.98); }

  .btn-start { background: var(--green); color: #fff; }
  .btn-stop  { background: var(--dim);   color: var(--text); }

  /* ── Speed levels ── */
  .speed-btns {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 8px;
    width: 100%;
    max-width: 360px;
  }

  .speed-btn {
    padding: 14px 0;
    border: none;
    border-radius: 10px;
    font-size: 18px;
    font-weight: 700;
    cursor: pointer;
    background: var(--card);
    color: var(--muted);
    border: 1px solid var(--border);
    transition: all 0.15s;
  }

  .speed-btn.aktiv {
    background: var(--green);
    color: #fff;
    border-color: var(--green);
  }

  .speed-btn:active { transform: scale(0.95); }

  /* ── Parameters ── */
  .param-row {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 6px 0;
    border-bottom: 1px solid var(--border);
  }

  .param-row input[type=number]:disabled {
    opacity: 0.6;
    cursor: not-allowed;
  }

  .param-row:last-child { border-bottom: none; }

  .param-label {
    font-size: 13px;
    color: var(--muted);
  }

  .param-row input[type=number] {
    width: 80px;
    background: var(--bg);
    border: 1px solid var(--border);
    border-radius: 8px;
    color: var(--text);
    font-size: 14px;
    font-weight: 500;
    padding: 6px 10px;
    text-align: right;
    outline: none;
  }

  .param-row input[type=number]:focus {
    border-color: var(--green);
  }

  .btn-param {
    background: var(--card);
    color: var(--muted);
    font-size: 14px;
    padding: 12px;
    border: 1px solid var(--border);
  }

  #feedback {
    font-size: 12px;
    color: var(--muted);
    text-align: center;
    min-height: 18px;
  }

  .section-label {
    font-size: 11px;
    font-weight: 600;
    letter-spacing: 1px;
    text-transform: uppercase;
    color: var(--muted);
    width: 100%;
    max-width: 360px;
    padding-left: 4px;
  }

  /* ── Landscape orientation ──────────────────────────────
     .layout/.col/.btn-row behave as "display: contents" in
     portrait mode – the elements remain direct flex children
     of body and the order/appearance is unchanged from the
     previous portrait view. Only the media query below
     activates the two-column layout for wide/flat screens. */
  .layout,
  .col,
  .btn-row {
    display: contents;
  }

  @media (orientation: landscape) and (max-height: 500px) {
    body {
      padding: 12px 20px;
      gap: 8px;
      justify-content: flex-start;
    }

    h1 {
      font-size: 18px;
      letter-spacing: 2px;
      margin-bottom: 0;
    }

    .layout {
      display: flex;
      flex-direction: row;
      align-items: flex-start;
      justify-content: center;
      gap: 16px;
      width: 100%;
      max-width: 640px;
    }

    .col {
      display: flex;
      flex-direction: column;
      gap: 8px;
    }

    .col-status  { flex: 0 0 220px; }
    .col-controls { flex: 1 1 300px; max-width: 320px; }

    .card {
      padding: 10px 14px;
    }

    .sensor-value {
      font-size: 15px;
    }

    .btn-row {
      display: flex;
      flex-direction: row;
      gap: 8px;
    }

    .btn-row .btn {
      width: auto;
      max-width: none;
      flex: 1;
      padding: 10px;
    }

    #feedback {
      min-height: 14px;
    }
  }
</style>
</head>
<body>

<h1>KIRBY</h1>

<div class="layout">

  <div class="col col-status">
    <!-- Status card -->
    <div class="card">
      <div class="card-title">Status
      <span id="state-badge" class="badge-init">INIT</span>
       </div>
      <div class="sensor-grid">
        <div class="sensor-item">
          <div class="sensor-label">Links</div>
          <div class="sensor-value" id="dist-l">–</div>
        </div>
        <div class="sensor-item">
          <div class="sensor-label">Mitte</div>
          <div class="sensor-value" id="dist-m">–</div>
        </div>
        <div class="sensor-item">
          <div class="sensor-label">Rechts</div>
          <div class="sensor-value" id="dist-r">–</div>
        </div>
      </div>
    </div>
  </div>

  <div class="col col-controls">
    <!-- Controls -->
    <span class="section-label">Steuerung</span>
    <div class="btn-row">
      <button class="btn btn-start" onclick="sende('/start')">Start</button>
      <button class="btn btn-stop"  onclick="sende('/stopp')">Stopp</button>
    </div>

    <!-- Speed levels -->
    <span class="section-label">Geschwindigkeit</span>
    <div class="speed-btns">
      <button class="speed-btn aktiv" id="s1" onclick="setzeStufe(1)">1</button>
      <button class="speed-btn"       id="s2" onclick="setzeStufe(2)">2</button>
      <button class="speed-btn"       id="s3" onclick="setzeStufe(3)">3</button>
    </div>

    <!-- Parameters -->
    <span class="section-label">Abstimmung</span>
    <div class="card">
      <div class="param-row">
        <span class="param-label">Turn-Schwelle (cm)</span>
        <input type="number" id="p-turn" min="1" max="400">
      </div>
      <div class="param-row">
        <span class="param-label">Stop-Schwelle (cm)</span>
        <input type="number" id="p-stop" min="1" max="400">
      </div>
      <div class="param-row">
        <span class="param-label">Seiten-Schwelle (cm)</span>
        <input type="number" id="p-side" min="1" max="400">
      </div>
      <div class="param-row">
        <span class="param-label">Kurvenfaktor (%)</span>
        <input type="number" id="p-turnfactor" min="-100" max="100">
      </div>
      <div class="param-row">
        <span class="param-label">Rückwärts-Dauer (ms)</span>
        <input type="number" id="p-rev" min="50" max="5000" step="50">
      </div>
    </div>
    <button class="btn btn-param" onclick="paramSenden()">↺ Parameter übernehmen</button>

    <div id="feedback"></div>
  </div>

</div>

<script>
// ── Helper functions ────────────────────────────────────────

const $ = id => document.getElementById(id);

function feedback(msg, color = '#666') {
  const el = $('feedback');
  el.style.color = color;
  el.textContent = msg;
}

async function sende(pfad) {
  try {
    await fetch(pfad);
    feedback('✓ ' + pfad, '#1D9E75');
  } catch(e) {
    feedback('Fehler: ' + e, '#E24B4A');
  }
}

// ── Speed levels ──────────────────────────────────────────────

async function setzeStufe(n) {
  try {
    await fetch('/stufe?n=' + n);
    for (let i = 1; i <= 3; i++) {
      $('s' + i).classList.toggle('aktiv', i === n);
    }
    feedback('Stufe ' + n + ' aktiv', '#1D9E75');
  } catch(e) {
    feedback('Fehler: ' + e, '#E24B4A');
  }
}

// ── Send parameters ──────────────────────────────────────────

async function paramSenden() {
  const turn       = $('p-turn').value;
  const side       = $('p-side').value;
  const stop       = $('p-stop').value;
  const turnFactor = $('p-turnfactor').value;
  const revDur     = $('p-rev').value;
  try {
    await fetch(`/parameter?turn=${turn}&sideTurn=${side}&stop=${stop}&turnFactor=${turnFactor}&revDuration=${revDur}`);
    feedback('Parameter gespeichert', '#1D9E75');
  } catch(e) {
    feedback('Fehler: ' + e, '#E24B4A');
  }
}

// ── Status parser ────────────────────────────────────────────
// Expects: "State:N Turn:N Stop:N Side:N TF:N Rev:N Speed:N Lvl:N M:N L:N R:N BTM:N"

// Robot::State only knows INIT(0) / READY(1) / RUN(2) – see Robot.h.
// A fourth entry would never be reachable here and has been removed.
const STATE_LABELS = ['INIT', 'READY', 'RUN'];
const STATE_CLASSES = ['badge-init', 'badge-ready', 'badge-run'];

function parseStatus(raw) {
  const kv = {};
  raw.trim().split(' ').forEach(pair => {
    const [k, v] = pair.split(':');
    if (k && v !== undefined) kv[k] = parseInt(v, 10);
  });
  return kv;
}

function distColor(v) {
  if (v >= 999) return '';
  if (v < 30)   return 'alert';
  if (v < 100)  return 'warn';
  return 'ok';
}

// ── Keep parameter fields in sync with the firmware ──────────
// The single source of truth is the live value from /status
// (ultimately Config.h, or the value most recently set via
// /parameter). No hardcoded default needed in the HTML anymore -
// this way no discrepancy between firmware and UI can arise,
// even if the Config.h defaults change later on.
//
// While the user is currently typing in a field (has focus),
// the value is not overwritten, so the 800ms polling doesn't
// clobber the input.
function syncParamField(id, value) {
  if (value === undefined) return;
  const el = $(id);
  if (document.activeElement === el) return;
  el.value = value;
}

function updateUI(kv) {
  // State badge
  const stateIdx = isNaN(kv.State) ? 0 : Math.min(kv.State, STATE_LABELS.length - 1);
  const badge = $('state-badge');
  badge.textContent = STATE_LABELS[stateIdx];
  badge.className = STATE_CLASSES[stateIdx];

  // Synchronize parameter fields (turn/stop threshold) live
  syncParamField('p-turn', kv.Turn);
  syncParamField('p-stop', kv.Stop);
  syncParamField('p-side', kv.Side);
  syncParamField('p-turnfactor', kv.TF);
  syncParamField('p-rev', kv.Rev);

  // Distances
  ['m', 'l', 'r'].forEach(dir => {
    const el = $('dist-' + dir);
    const key = dir.toUpperCase();
    const v = kv[key];
    if (v === undefined) return;
    el.textContent = (v >= 999) ? '–' : v + ' cm';
    el.className = 'sensor-value ' + distColor(v);
  });

  // Determine active level
  const speed = kv.Speed;
  const levels = [165, 205, 245];
  const activeLevel = levels.indexOf(speed) + 1;
  if (activeLevel > 0) {
    for (let i = 1; i <= 3; i++) {
      $('s' + i).classList.toggle('aktiv', i === activeLevel);
    }
  }
}

// ── Auto-refresh every 800ms ─────────────────────────────────

async function refresh() {
  try {
    const r = await fetch('/status');
    const t = await r.text();
    updateUI(parseStatus(t));
  } catch(e) {
    // Connection briefly interrupted – don't show an error message
  }
}

setInterval(refresh, 800);
refresh(); // immediately on load
</script>
</body>
</html>
)rawliteral";