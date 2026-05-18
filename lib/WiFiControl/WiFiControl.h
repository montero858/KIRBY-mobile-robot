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
  #btn-notstopp { background: #E24B4A; color: #fff; font-size: 20px; }
  #btn-reset    { background: #222;    color: #aaa; font-size: 14px; padding: 12px; }
  .divider { width: 100%; max-width: 340px; border: none;
             border-top: 1px solid #333; }
  /* Trimm-Regler */
  .trim-box { background: #1e1e1e; border-radius: 12px;
              padding: 16px 20px; width: 100%; max-width: 340px; }
  .trim-label { display: flex; justify-content: space-between;
                font-size: 13px; color: #aaa; margin-bottom: 10px; }
  .trim-label span { font-size: 13px; color: #eee; font-weight: 500; }
  .trim-row { display: flex; align-items: center; gap: 10px; }
  .trim-row small { font-size: 11px; color: #555; min-width: 24px; text-align: center; }
  input[type=range] { flex: 1; accent-color: #1D9E75; }
</style>
</head>
<body>

<h1>HENRY</h1>

<div id="status-box">
  <div id="status">Verbunden</div>
  <div id="abstand">Abstand: – cm</div>
</div>

<button class="btn" id="btn-start"    onclick="sende('/start')">▶ Start</button>
<button class="btn" id="btn-stop"     onclick="sende('/stopp')">⏹ Stopp</button>
<hr class="divider">
<button class="btn" id="btn-notstopp" onclick="sende('/notstopp')">⛔ NOT-STOPP</button>
<button class="btn" id="btn-reset"    onclick="sende('/reset')">↺ Reset (Not-Stopp aufheben)</button>

<!-- Trimm-Regler -->
<div class="trim-box">
  <div class="trim-label">
    <span>Motor-Trimm</span>
    <span id="trim-wert">0</span>
  </div>
  <div class="trim-row">
    <small>L</small>
    <input type="range" id="trimm" min="-20" max="20" value="0"
           oninput="trimmAktualisieren(this.value)"
           onchange="trimmSenden(this.value)">
    <small>R</small>
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
function trimmAktualisieren(wert) {
  document.getElementById('trim-wert').textContent = wert;
}

// Wert erst senden wenn Finger losgelassen wird (onchange)
async function trimmSenden(wert) {
  try {
    await fetch('/trimm?wert=' + wert);
  } catch(e) {
    document.getElementById('status').textContent = 'Trimm-Fehler: ' + e;
  }
}

// Abstand alle 1,5 Sekunden automatisch aktualisieren
setInterval(async () => {
  try {
    const r = await fetch('/abstand');
    const t = await r.text();
    document.getElementById('abstand').textContent = 'Abstand: ' + t + ' cm';
  } catch(e) {}
}, 1500);
</script>
</body>
</html>
)rawliteral";

#endif // WIFICONTROL_H
