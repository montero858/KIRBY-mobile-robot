// ============================================================
//  WiFiControl.h – HTML-Steuerseite fuer den Roboter
// ============================================================

#ifndef WIFICONTROL_H
#define WIFICONTROL_H

const char HTML_SEITE[] = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Roboter</title>
<style>
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body { font-family: sans-serif; background: #111; color: #eee;
         display: flex; flex-direction: column; align-items: center;
         min-height: 100vh; padding: 24px 16px; gap: 20px; }
  h1   { font-size: 22px; font-weight: 500; letter-spacing: 1px; }
  #status-box { background: #1e1e1e; border-radius: 12px;
                padding: 14px 24px; text-align: center;
                width: 100%; max-width: 340px; }
  #status  { font-size: 15px; color: #aaa; margin-top: 4px; }
  #abstand { font-size: 12px; color: #666; margin-top: 6px; }
  .btn { width: 100%; max-width: 340px; padding: 18px; border: none;
         border-radius: 12px; font-size: 18px; font-weight: 500;
         cursor: pointer; transition: opacity .15s; }
  .btn:active { opacity: .75; }
  #btn-start    { background: #1D9E75; color: #fff; }
  #btn-stop     { background: #444;    color: #eee; }
  #btn-scan     { background: #378ADD; color: #fff; }
  .speed-box { background: #1e1e1e; border-radius: 12px;
               padding: 16px 20px; width: 100%; max-width: 340px; }
  .speed-box h2 { font-size: 13px; font-weight: 500; color: #aaa; margin-bottom: 12px; }
  .speed-btns { display: grid; grid-template-columns: repeat(4, 1fr); gap: 8px; }
  .speed-btn { padding: 14px 0; border: none; border-radius: 8px; font-size: 16px;
               font-weight: 600; cursor: pointer; background: #2a2a2a; color: #666;
               transition: all .15s; }
  .speed-btn.aktiv { background: #1D9E75; color: #fff; }
  .param-box { background: #1e1e1e; border-radius: 12px;
               padding: 16px 20px; width: 100%; max-width: 340px; }
  .param-box h2 { font-size: 13px; font-weight: 500; color: #aaa; margin-bottom: 12px; }
  .param-row { display: flex; justify-content: space-between;
               align-items: center; margin-bottom: 8px; }
  .param-row label { font-size: 12px; color: #666; flex: 1; }
  .param-row input[type=number] { width: 72px; background: #2a2a2a; border: 1px solid #444;
                                   border-radius: 6px; color: #eee; font-size: 13px;
                                   padding: 4px 8px; text-align: right; }
  #btn-param { background: #333; color: #aaa; font-size: 14px; padding: 12px; }
</style>
</head>
<body>

<h1>KIRBY</h1>

<div id="status-box">
  <div id="status">Verbunden</div>
  <div id="abstand">–</div>
</div>

<button class="btn" id="btn-start" onclick="sende('/start')">▶ Start</button>
<button class="btn" id="btn-stop"  onclick="sende('/stopp')">⏹ Stopp</button>
<button class="btn" id="btn-scan"  onclick="sende('/scan')">⟳ Neu scannen</button>

<div class="speed-box">
  <h2>Geschwindigkeitsstufe</h2>
  <div class="speed-btns">
    <button class="speed-btn aktiv" id="s1" onclick="setzeStufe(1)">1</button>
    <button class="speed-btn"       id="s2" onclick="setzeStufe(2)">2</button>
    <button class="speed-btn"       id="s3" onclick="setzeStufe(3)">3</button>
    <button class="speed-btn"       id="s4" onclick="setzeStufe(4)">4</button>
  </div>
</div>

<div class="param-box">
  <h2>Abstimmung</h2>
  <div class="param-row">
    <label>Front Slow (cm)</label>
    <input type="number" id="p-fs" min="1" max="400" value="100">
  </div>
  <div class="param-row">
    <label>Front Stop (cm)</label>
    <input type="number" id="p-fp" min="1" max="400" value="15">
  </div>
  <div class="param-row">
    <label>Drehgeschwindigkeit (0-255)</label>
    <input type="number" id="p-dg" min="0" max="255" value="200">
  </div>
</div>
<button class="btn" id="btn-param" onclick="parameterSenden()">↺ Parameter übernehmen</button>

<script>
async function sende(pfad) {
  try {
    const r = await fetch(pfad);
    const t = await r.text();
    document.getElementById('status').textContent = t;
  } catch(e) {
    document.getElementById('status').textContent = 'Fehler: ' + e;
  }
}

async function setzeStufe(stufe) {
  try {
    await fetch('/stufe?wert=' + stufe);
    for (let i = 1; i <= 4; i++) {
      document.getElementById('s' + i).classList.toggle('aktiv', i === stufe);
    }
    document.getElementById('status').textContent = 'Stufe ' + stufe + ' aktiv';
  } catch(e) {
    document.getElementById('status').textContent = 'Fehler: ' + e;
  }
}

async function parameterSenden() {
  const fs = document.getElementById('p-fs').value;
  const fp = document.getElementById('p-fp').value;
  const dg = document.getElementById('p-dg').value;
  try {
    const r = await fetch(`/parameter?fs=${fs}&fp=${fp}&dg=${dg}`);
    const json = await r.json();
    document.getElementById('p-fs').value = json.fs;
    document.getElementById('p-fp').value = json.fp;
    document.getElementById('p-dg').value = json.dg;
    document.getElementById('status').textContent = 'Parameter gespeichert';
  } catch(e) {
    document.getElementById('status').textContent = 'Fehler: ' + e;
  }
}

window.addEventListener('load', async () => {
  try {
    const r = await fetch('/parameter');
    const json = await r.json();
    document.getElementById('p-fs').value = json.fs;
    document.getElementById('p-fp').value = json.fp;
    document.getElementById('p-dg').value = json.dg;
  } catch(e) {}
});

setInterval(async () => {
  try {
    const r = await fetch('/abstand');
    const t = await r.text();
    document.getElementById('abstand').textContent = t;
  } catch(e) {}
}, 1000);
</script>
</body>
</html>
)rawliteral";

#endif // WIFICONTROL_H