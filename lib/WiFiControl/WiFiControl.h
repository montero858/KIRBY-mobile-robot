// ============================================================
//  WiFiControl.h – HTML-Steuerseite fuer den Roboter
//  Hier das Layout und Design der App anpassen
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
  #abstand { font-size: 13px; color: #666; margin-top: 6px; }
  .btn { width: 100%; max-width: 340px; padding: 18px; border: none;
         border-radius: 12px; font-size: 18px; font-weight: 500;
         cursor: pointer; transition: opacity .15s; }
  .btn:active { opacity: .75; }
  #btn-start    { background: #1D9E75; color: #fff; }
  #btn-stop     { background: #444;    color: #eee; }
  /* Parameter-Eingabe */
  .param-box { background: #1e1e1e; border-radius: 12px;
               padding: 16px 20px; width: 100%; max-width: 340px; }
  .param-box h2 { font-size: 13px; font-weight: 500; color: #aaa;
                  margin-bottom: 12px; }
  .param-row { display: flex; justify-content: space-between;
               align-items: center; margin-bottom: 8px; }
  .param-row label { font-size: 12px; color: #666; flex: 1; }
  .param-row input[type=number] { width: 72px; background: #2a2a2a; border: 1px solid #444;
                                   border-radius: 6px; color: #eee; font-size: 13px;
                                   padding: 4px 8px; text-align: right; }
  #btn-param { background: #333; color: #aaa; font-size: 14px; padding: 12px; }
  .speed-box { background: #1e1e1e; border-radius: 12px;
              padding: 16px 20px; width: 100%; max-width: 340px; }
  .speed-label { display: flex; justify-content: space-between;
                font-size: 13px; color: #aaa; margin-bottom: 10px; }
  .speed-label span { font-size: 13px; color: #eee; font-weight: 500; }
  .speed-row { display: flex; align-items: center; gap: 10px; }
  .speed-row small { font-size: 11px; color: #555; min-width: 24px; text-align: center; }
  input[type=range] { flex: 1; accent-color: #1D9E75; }
</style>
</head>
<body>

<h1>KIRBY</h1>

<div id="status-box">
  <div id="status">Verbunden</div>
  <div id="abstand">Abstand: – cm</div>
</div>

<button class="btn" id="btn-start" onclick="sende('/start')">▶ Start</button>
<button class="btn" id="btn-stop"  onclick="sende('/stopp')">⏹ Stopp</button>

<!-- Parameter-Eingabe -->
<div class="param-box">
  <h2>Abstimmung</h2>
  <div class="param-row">
    <label>Front Limit (cm)</label>
    <input type="number" id="p-fl" min="1" max="400" value="40">
  </div>
  <div class="param-row">
    <label>Front Stop (cm)</label>
    <input type="number" id="p-fs" min="1" max="400" value="15">
  </div>
  <div class="param-row">
    <label>Seite Limit (cm)</label>
    <input type="number" id="p-sl" min="1" max="400" value="25">
  </div>
  <div class="param-row">
    <label>Seite Stop (cm)</label>
    <input type="number" id="p-ss" min="1" max="400" value="10">
  </div>
  <div class="param-row">
    <label>Drehgeschwindigkeit (0-255)</label>
    <input type="number" id="p-dg" min="0" max="255" value="200">
  </div>
  <div class="param-row">
    <label>Kurve innen (%)</label>
    <input type="number" id="p-kp" min="0" max="100" value="60">
  </div>
</div>
<button class="btn" id="btn-param" onclick="parameterSenden()">↺ Parameter übernehmen</button>
<div class="speed-box">
  <div class="speed-label">
    <span>Geschwindigkeit</span>
    <span id="geschwindigkeit-wert">0</span>
  </div>
  <div class="speed-row">
    <input type="range" id="geschwindigkeit" min="0" max="255" value="122"
           oninput="geschwindigkeitAktualisieren(this.value)"
           onchange="geschwindigkeitSenden(this.value)">
  </div>
</div>

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

// Wert live anzeigen waehrend Regler bewegt wird
function geschwindigkeitAktualisieren(wert) {
  document.getElementById('geschwindigkeit-wert').textContent = wert;
}

// Wert erst senden wenn Finger losgelassen wird (onchange)
async function geschwindigkeitSenden(wert) {
  try {
    await fetch('/geschwindigkeit?wert=' + wert);
  } catch(e) {
    document.getElementById('status').textContent = 'Geschwindigkeit-Fehler: ' + e;
  }
}

async function parameterSenden() {
  const fl = document.getElementById('p-fl').value;
  const fs = document.getElementById('p-fs').value;
  const sl = document.getElementById('p-sl').value;
  const ss = document.getElementById('p-ss').value;
  const dg = document.getElementById('p-dg').value;
  const kp = document.getElementById('p-kp').value;
  try {
    const r = await fetch(`/parameter?fl=${fl}&fs=${fs}&sl=${sl}&ss=${ss}&dg=${dg}&kp=${kp}`);
    const json = await r.json();
    // Felder mit bestaetigen Werten vom ESP32 aktualisieren
    document.getElementById('p-fl').value = json.fl;
    document.getElementById('p-fs').value = json.fs;
    document.getElementById('p-sl').value = json.sl;
    document.getElementById('p-ss').value = json.ss;
    document.getElementById('p-dg').value = json.dg;
    document.getElementById('p-kp').value = json.kp;
    document.getElementById('status').textContent = 'Parameter gespeichert';
  } catch(e) {
    document.getElementById('status').textContent = 'Fehler: ' + e;
  }
}

// Aktuelle Parameter beim Laden der Seite vom ESP32 holen
window.addEventListener('load', async () => {
  try {
    const r = await fetch('/parameter');
    const json = await r.json();
    document.getElementById('p-fl').value = json.fl;
    document.getElementById('p-fs').value = json.fs;
    document.getElementById('p-sl').value = json.sl;
    document.getElementById('p-ss').value = json.ss;
    document.getElementById('p-dg').value = json.dg;
    document.getElementById('p-kp').value = json.kp;
  } catch(e) {}
});
setInterval(async () => {
  try {
    const r = await fetch('/abstand');
    const t = await r.text();
    document.getElementById('abstand').textContent = t;
  } catch(e) {}
}, 1500);
</script>
</body>
</html>
)rawliteral";

#endif // WIFICONTROL_H