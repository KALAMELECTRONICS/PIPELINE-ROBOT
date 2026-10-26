#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>

const char* WIFI_SSID = "Kalam_004";
const char* WIFI_PASSWORD = "kalam@2026";

#define IN1 26
#define IN2 27
#define ENA 25

#define IN3 18
#define IN4 19
#define ENB 5

#define DHT_PIN 4
#define DHT_TYPE DHT11

#define VIBRATION_PIN 34

DHT dht(DHT_PIN, DHT_TYPE);
WebServer server(80);

int motorSpeed = 150;
String robotState = "STOPPED";
float temperature = NAN;
float humidity = NAN;
int vibrationValue = 0;
bool motionActive = false;
unsigned long lastCommandMs = 0;
unsigned long lastDhtMs = 0;
const unsigned long SAFETY_TIMEOUT_MS = 2500;

void stopMotors() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  robotState = "STOPPED";
  motionActive = false;
  Serial.println("[MOTOR] STOPPED");
}

void setMotion(String cmd) {
  cmd.toUpperCase();
  if (cmd == "STOP") { stopMotors(); return; }
  if (cmd == "FORWARD") {
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  } else if (cmd == "REVERSE") {
    digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  } else if (cmd == "LEFT") {
    digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
    digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  } else if (cmd == "RIGHT") {
    digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
  } else { return; }

  analogWrite(ENA, motorSpeed);
  analogWrite(ENB, motorSpeed);
  robotState = cmd;
  motionActive = (motorSpeed > 0);
  lastCommandMs = millis();
  Serial.printf("[MOTOR] %s speed=%d\n", cmd.c_str(), motorSpeed);
}

const char DASHBOARD_HTML[] PROGMEM = R"PGPAGE(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="theme-color" content="#ffffff">
<title>PipeGuard | Pipeline Intelligence</title>
<style>
:root{
 --bg:#f3f6fb;--sidebar:#ffffff;--panel:#ffffff;--panel-2:#f8fafc;--border:#dce4ee;
 --text:#17263b;--muted:#64748b;--teal:#0d9488;--blue:#2563eb;--green:#16a34a;
 --amber:#d97706;--red:#dc2626;--shadow:0 8px 24px #1e293b0d;
}
*{box-sizing:border-box}
body{margin:0;background:linear-gradient(180deg,#f8fafc,var(--bg));color:var(--text);font:14px/1.45 Inter,ui-sans-serif,system-ui,-apple-system,"Segoe UI",sans-serif}
button,input{font:inherit}button{cursor:pointer}
.app{min-height:100vh;display:grid;grid-template-columns:248px minmax(0,1fr)}
.sidebar{background:#ffffff;border-right:1px solid var(--border);padding:24px 16px;display:flex;flex-direction:column;gap:28px}
.brand{display:flex;align-items:center;gap:12px;padding:0 8px}
.brand-mark{height:42px;width:42px;border-radius:14px;background:linear-gradient(145deg,#3ee0cd,#5da6ff);display:grid;place-items:center;color:#061421;font-weight:900;font-size:20px;box-shadow:0 8px 24px #42d9c833}
.brand-name{font-size:17px;font-weight:800;letter-spacing:.2px}.brand-sub{font-size:10px;color:var(--muted);letter-spacing:1.6px;margin-top:3px}
.nav-label{font-size:10px;color:#7187a1;letter-spacing:1.8px;padding:0 12px 8px;text-transform:uppercase}
.nav{display:grid;gap:6px}.nav a{display:flex;gap:12px;align-items:center;color:#475569;text-decoration:none;padding:12px;border-radius:11px;border:1px solid transparent;font-weight:600}
.nav a.active,.nav a:hover{background:#eff6ff;border-color:#bfdbfe;color:#1d4ed8}.nav-icon{width:20px;text-align:center;font-size:17px}
.sidebar-bottom{margin-top:auto}.connection-card{padding:14px;border-radius:14px;background:#f8fafc;border:1px solid #dce4ee}
.connection-head{display:flex;align-items:center;gap:8px;font-size:12px;font-weight:700}.status-dot{width:8px;height:8px;border-radius:50%;background:var(--amber);box-shadow:0 0 0 4px #ffc66d16}
.connection-card p{margin:9px 0 0;color:var(--muted);font-size:11px;line-height:1.6}
main{min-width:0;padding:28px clamp(16px,3vw,38px) 36px;max-width:1680px;width:100%;margin:0 auto}
.topbar{display:flex;align-items:center;justify-content:space-between;gap:18px;flex-wrap:wrap;margin-bottom:28px}
.eyebrow{font-size:10px;letter-spacing:2px;color:var(--teal);text-transform:uppercase;font-weight:800;margin-bottom:7px}
h1{font-size:clamp(24px,3vw,32px);letter-spacing:-.9px;line-height:1.12;margin:0}
.subtitle{color:var(--muted);margin-top:8px;font-size:13px}
.top-actions{display:flex;align-items:center;gap:10px;flex-wrap:wrap}
.chip{display:inline-flex;align-items:center;gap:8px;border:1px solid var(--border);border-radius:999px;padding:9px 12px;background:#101d2d;color:#475569;font-size:11px;font-weight:700}
.chip .status-dot{background:var(--green);box-shadow:0 0 0 4px #45d48315}
.btn{border:1px solid #2a4560;background:#162b42;color:#17263b;padding:10px 13px;border-radius:10px;font-weight:700;font-size:12px;transition:.18s ease}
.btn:hover{transform:translateY(-1px);border-color:#4b7e9f}.btn-primary{background:linear-gradient(135deg,#0d9488,#3b82f6);border:0;color:#ffffff}.btn-danger{background:#fee2e2;border-color:#fecaca;color:#991b1b}
.summary-grid{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:14px;margin-bottom:18px}
.kpi{border:1px solid var(--border);border-radius:16px;background:linear-gradient(145deg,#ffffff,#f8fafc);padding:17px;box-shadow:var(--shadow);min-width:0}
.kpi-top{display:flex;align-items:center;justify-content:space-between;gap:8px;color:var(--muted);font-size:11px;font-weight:700}
.kpi-icon{height:32px;width:32px;display:grid;place-items:center;border-radius:10px;background:#e0f2fe;color:var(--teal);font-size:16px}
.kpi-value{font-size:25px;font-weight:800;letter-spacing:-.6px;margin:14px 0 4px;white-space:nowrap}.kpi-foot{font-size:10px;color:var(--muted)}.kpi-foot strong{color:var(--green);font-weight:700}
.content-grid{display:grid;grid-template-columns:minmax(300px,.82fr) minmax(0,1.45fr);gap:16px;align-items:stretch}
.view-hidden{display:none!important}.content-grid.single-view{grid-template-columns:minmax(0,1fr)}
.panel{min-width:0;background:linear-gradient(150deg,#ffffff,#fbfdff);border:1px solid var(--border);border-radius:18px;padding:20px;box-shadow:var(--shadow)}
.panel-head{display:flex;align-items:flex-start;justify-content:space-between;gap:12px;margin-bottom:18px}
.panel-title{font-size:15px;font-weight:800;letter-spacing:-.2px}.panel-desc{font-size:11px;color:var(--muted);margin-top:4px}
.panel-tag{font-size:10px;font-weight:800;letter-spacing:.7px;color:#475569;border:1px solid #2b4058;border-radius:7px;padding:6px 8px;white-space:nowrap}
.robot-stage{border:1px solid #203650;border-radius:15px;background:radial-gradient(ellipse at 50% 0%,#f1f8ff,#ffffff 75%);padding:18px 12px}
.dpad{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:9px;max-width:280px;margin:0 auto}
.dpad button{min-height:64px;border:1px solid #304c68;border-radius:13px;background:#f1f5f9;color:#1e293b;font-weight:800;font-size:11px;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:3px;touch-action:manipulation;transition:.15s}
.dpad button .arrow{font-size:21px;line-height:1}.dpad button:hover{background:#e0f2fe;border-color:#7dd3fc}.dpad button:active{transform:scale(.96)}
.dpad button.go{background:#ccfbf1;border-color:#5eead4;color:#115e59}.dpad button.stop{background:#fee2e2;border-color:#fca5a5;color:#991b1b}
.dpad .empty{visibility:hidden}
.speed-block{margin-top:18px;padding:14px;border:1px solid #243b53;border-radius:13px;background:#f8fafc}
.speed-head{display:flex;justify-content:space-between;gap:8px;align-items:center;font-size:11px;color:var(--muted)}.speed-number{color:var(--teal);font-size:16px;font-weight:800}
.speed-track{height:5px;background:#e2e8f0;border-radius:99px;margin-top:12px;overflow:hidden}.speed-track span{display:block;width:0;height:100%;background:linear-gradient(90deg,#38d4c4,#6baaff);transition:width .2s}
.robot-foot{display:flex;justify-content:space-between;align-items:center;gap:8px;margin-top:14px;font-size:11px;color:var(--muted)}.state-pill{padding:6px 9px;border-radius:8px;background:#dcfce7;color:#166534;font-weight:800;font-size:10px;letter-spacing:.6px}
.camera-box{background:#eef2f7;border:1px solid #263a50;border-radius:13px;overflow:hidden;aspect-ratio:16/9;display:grid;place-items:center;position:relative}
.camera-box img{display:block;width:100%;height:100%;object-fit:contain}.camera-placeholder{text-align:center;color:#64748b;padding:20px}.camera-placeholder .camera-glyph{font-size:36px;color:#48d8cd}.camera-placeholder b{display:block;color:#17263b;margin:8px 0}.camera-overlay{position:absolute;top:12px;left:12px;display:flex;gap:7px;align-items:center;background:#ffffffed;border:1px solid #dce4ee;padding:7px 9px;border-radius:7px;font-size:10px;font-weight:800;letter-spacing:.6px}
.live-dot{width:6px;height:6px;border-radius:50%;background:var(--red)}.camera-toolbar{display:flex;justify-content:space-between;align-items:center;gap:10px;flex-wrap:wrap;margin-top:13px}.camera-meta{font-size:10px;color:var(--muted)}
.sensor-section{margin-top:16px}.sensor-grid{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:12px}
.sensor{border:1px solid var(--border);border-radius:14px;background:#f8fafc;padding:15px;min-width:0}
.sensor-top{display:flex;justify-content:space-between;gap:8px;align-items:center}.sensor-name{font-size:11px;color:#475569;font-weight:700}.sensor-icon{color:var(--teal);font-size:17px}
.sensor-value{font-size:24px;font-weight:800;letter-spacing:-.5px;margin:15px 0 3px;overflow-wrap:anywhere}.sensor-unit{font-size:10px;color:var(--muted)}.sensor-note{font-size:10px;color:#64748b;margin-top:13px}
.mini-bar{height:4px;border-radius:99px;background:#e2e8f0;overflow:hidden;margin-top:10px}.mini-bar span{display:block;height:100%;width:0;background:var(--teal);transition:width .3s}
.alert-strip{display:flex;align-items:flex-start;gap:11px;border:1px solid #315147;background:#102821;border-radius:13px;padding:13px 14px;margin-top:14px;color:#b4e9d4;font-size:11px;line-height:1.6}
.alert-icon{font-size:16px;color:var(--green)}
.bottom-line{display:flex;justify-content:space-between;gap:12px;flex-wrap:wrap;margin-top:20px;color:#64748b;font-size:10px}
@media(max-width:1120px){.app{grid-template-columns:210px minmax(0,1fr)}.summary-grid{grid-template-columns:repeat(2,minmax(0,1fr))}.content-grid{grid-template-columns:1fr}.sensor-grid{grid-template-columns:repeat(2,minmax(0,1fr))}}
@media(max-width:720px){.app{display:block}.sidebar{padding:14px 16px;border-right:0;border-bottom:1px solid var(--border);gap:14px}.brand{padding:0}.nav-label,.sidebar-bottom{display:none}.nav{display:flex;overflow:auto}.nav a{white-space:nowrap;padding:9px 11px;font-size:11px}.nav-icon{font-size:14px}.topbar{margin-bottom:20px}main{padding-top:20px}.summary-grid{gap:10px}.kpi{padding:13px}.kpi-value{font-size:22px}.panel{padding:15px}.sensor-grid{gap:9px}.sensor{padding:12px}}
@media(max-width:390px){.summary-grid{grid-template-columns:1fr 1fr}.kpi-value{font-size:19px}.sensor-value{font-size:21px}.chip{padding:8px 9px}}

:root{
  --bg:#f1f5f9;
  --sidebar:#ffffff;
  --panel:#ffffff;
  --panel-2:#f8fafc;
  --border:#cbd5e1;
  --text:#0f172a;
  --muted:#475569;
  --teal:#0f766e;
  --blue:#1d4ed8;
  --green:#15803d;
  --amber:#b45309;
  --red:#b91c1c;
  --shadow:0 6px 20px rgba(15,23,42,.08);
}
body{color:#0f172a;background:#f1f5f9}
.brand-sub,.nav-label,.panel-desc,.subtitle,.kpi-foot,.sensor-unit,.sensor-note,.camera-meta,.bottom-line{color:#475569}
.nav a{color:#334155}
.nav a.active,.nav a:hover{background:#dbeafe;border-color:#93c5fd;color:#1e40af}
.connection-card{background:#f8fafc;border-color:#cbd5e1}
.connection-head{color:#0f172a}
.chip{background:#f8fafc;color:#0f172a;border-color:#cbd5e1}
.chip .status-dot{background:#15803d}
.btn{background:#ffffff;color:#0f172a;border:1px solid #94a3b8;box-shadow:0 1px 2px rgba(15,23,42,.06)}
.btn:hover{background:#eff6ff;color:#1e40af;border-color:#60a5fa}
.btn:focus-visible,.dpad button:focus-visible,input:focus-visible{outline:3px solid #2563eb;outline-offset:2px}
.btn-primary{background:#1d4ed8;color:#ffffff;border-color:#1d4ed8}
.btn-primary:hover{background:#1e40af;color:#ffffff;border-color:#1e40af}
.btn-danger{background:#fee2e2;border-color:#fca5a5;color:#991b1b}
.panel{background:#ffffff;border-color:#cbd5e1}
.panel-tag{background:#f1f5f9;color:#334155;border-color:#cbd5e1}
.robot-stage{background:linear-gradient(180deg,#f8fafc,#eef2ff);border-color:#cbd5e1}
.dpad button{background:#ffffff;color:#0f172a;border-color:#94a3b8}
.dpad button:hover{background:#dbeafe;border-color:#60a5fa;color:#1e40af}
.dpad button.go{background:#ccfbf1;border-color:#5eead4;color:#115e59}
.dpad button.stop{background:#fee2e2;border-color:#fca5a5;color:#991b1b}
.speed-block{background:#f8fafc;border-color:#cbd5e1}
.speed-head{color:#334155}
.speed-number{color:#0f766e}
.speed-track,.mini-bar{background:#cbd5e1}
.state-pill{background:#dcfce7;color:#166534}
.camera-box{background:#e2e8f0;border-color:#cbd5e1}
.camera-placeholder{color:#475569}
.camera-placeholder b{color:#0f172a}
.camera-overlay{background:rgba(255,255,255,.96);color:#0f172a;border-color:#cbd5e1}
.camera-meta{color:#334155}
.camera-toolbar input{background:#ffffff;color:#0f172a;border-color:#94a3b8}
.sensor{background:#f8fafc;border-color:#cbd5e1}
.sensor-name{color:#334155}
.sensor-note{color:#475569}
.alert-strip{background:#ecfdf5;border-color:#86efac;color:#14532d}
.alert-icon{color:#15803d}
input[type=range]{cursor:pointer}
@media(max-width:720px){
  .camera-toolbar{align-items:stretch}
  .camera-meta{display:flex;flex-wrap:wrap;align-items:center;gap:8px}
  .camera-meta input{min-width:0;flex:1}
  .camera-toolbar>.btn{align-self:flex-start}
  .dpad button{min-height:58px}
}

</style>
</head>
<body>
<div class="app">
<aside class="sidebar">
  <div class="brand"><div class="brand-mark">P</div><div><div class="brand-name">PipeGuard</div><div class="brand-sub">PIPELINE INTELLIGENCE</div></div></div>
  <div><div class="nav-label">Workspace</div><nav class="nav">
    <a class="active" data-view="overview" href="#overview"><span class="nav-icon">▦</span>Overview</a>
    <a data-view="control" href="#control"><span class="nav-icon">⌘</span>Robot control</a>
    <a data-view="camera" href="#camera"><span class="nav-icon">▣</span>Live camera</a>
    <a data-view="sensors" href="#sensors"><span class="nav-icon">⌁</span>Sensor telemetry</a>
  </nav></div>
  <div class="sidebar-bottom"><div class="connection-card"><div class="connection-head"><span class="status-dot" id="sideDot"></span><span id="sideStatus">Waiting for ESP32</span></div><p>Mobile hotspot network<br>Dashboard: <span id="dashboardIp">detecting IP…</span><br><span id="lastSeen">Telemetry not received</span></p></div></div>
</aside>
<main id="overview">
  <div class="topbar"><div><div class="eyebrow">Autonomous inspection console</div><h1>Pipeline Operations</h1><div class="subtitle">Monitor robot movement, camera feed and environmental telemetry.</div></div>
  <div class="top-actions"><div class="chip"><span class="status-dot" id="topDot"></span><span id="connectionText">CONNECTING TO ROBOT</span></div><button class="btn" id="refreshBtn">↻ Refresh</button></div></div>
  <section class="summary-grid" id="overviewCards">
    <div class="kpi"><div class="kpi-top"><span>ROBOT STATUS</span><span class="kpi-icon">⌖</span></div><div class="kpi-value" id="kpiRobot">STOPPED</div><div class="kpi-foot">Motion controller state</div></div>
    <div class="kpi"><div class="kpi-top"><span>PIPE TEMPERATURE</span><span class="kpi-icon">♨</span></div><div class="kpi-value"><span id="kpiTemp">--</span><small style="font-size:13px;color:var(--muted)"> °C</small></div><div class="kpi-foot">DHT11 ambient reading</div></div>
    <div class="kpi"><div class="kpi-top"><span>VIBRATION</span><span class="kpi-icon">⌁</span></div><div class="kpi-value"><span id="kpiVib">--</span></div><div class="kpi-foot">SD30 analog reading · ADC counts</div></div>
    <div class="kpi"><div class="kpi-top"><span>ACOUSTIC INPUT</span><span class="kpi-icon">♫</span></div><div class="kpi-value" id="kpiAcoustic">--</div><div class="kpi-foot">Raw ADC counts · not calibrated SPL</div></div>
  </section>
  <section class="content-grid" id="controlCameraGrid">
    <article class="panel" id="control"><div class="panel-head"><div><div class="eyebrow">01 / Mobility</div><div class="panel-title">Robot Control</div><div class="panel-desc">Hold a direction to move; release to stop.</div></div><span class="panel-tag">L298N DRIVER</span></div>
      <div class="robot-stage"><div class="dpad">
        <button class="empty" tabindex="-1" aria-hidden="true"></button><button class="go" data-cmd="FORWARD"><span class="arrow">↑</span>FORWARD</button><button class="empty" tabindex="-1" aria-hidden="true"></button>
        <button data-cmd="LEFT"><span class="arrow">←</span>LEFT</button><button class="stop" data-cmd="STOP"><span class="arrow">■</span>STOP</button><button data-cmd="RIGHT"><span class="arrow">→</span>RIGHT</button>
        <button class="empty" tabindex="-1" aria-hidden="true"></button><button class="go" data-cmd="REVERSE"><span class="arrow">↓</span>REVERSE</button><button class="empty" tabindex="-1" aria-hidden="true"></button>
      </div></div>
      <div class="speed-block"><div class="speed-head"><span>WEB SPEED CONTROL</span><span class="speed-number"><span id="speedVal">--</span> / 255</span></div><div class="speed-track"><span id="speedBar"></span></div><div class="speed-head" style="margin-top:10px"><span>Dashboard slider</span><span id="speedPercent">--%</span></div>
      <label for="webSpeed" style="display:block;margin-top:12px;font-size:12px;color:var(--muted)">Adjust motor speed</label>
      <input id="webSpeed" type="range" min="0" max="255" value="150" step="5" style="width:100%;accent-color:var(--blue);margin:10px 0 4px">
      <div class="speed-head"><span>Slow · 0</span><span id="webSpeedValue">150 / 255</span><span>Fast · 255</span></div>
      <div id="speedMessage" class="muted" style="font-size:11px;margin-top:8px">Web speed control ready</div></div>
      <div class="robot-foot"><span>Current motion</span><span class="state-pill" id="robotState">STOPPED</span></div>
    </article>
    <article class="panel" id="camera"><div class="panel-head"><div><div class="eyebrow">02 / Visual inspection</div><div class="panel-title">Live Camera Feed</div><div class="panel-desc">ESP32-CAM visual monitoring of the pipeline surface.</div></div><span class="panel-tag" id="camTag">CAMERA IDLE</span></div>
      <div class="camera-box"><img id="cameraFeed" alt="ESP32-CAM live stream" hidden><div class="camera-placeholder" id="camPlaceholder"><div class="camera-glyph">▣</div><b>Camera feed unavailable</b><span id="camMessage">Connect to the PipeGuard local Wi-Fi network.</span><br><button class="btn btn-primary" id="connectCam" style="margin-top:14px">Connect camera</button></div><div class="camera-overlay"><span class="live-dot" id="liveDot"></span><span id="liveLabel">NO SIGNAL</span></div></div>
      <div class="camera-toolbar">
        <div class="camera-meta">CAMERA IP&nbsp; / &nbsp;
          <input id="cameraIpInput" type="text" placeholder="e.g. 192.168.1.105" style="width:170px;max-width:55vw;padding:8px;border:1px solid var(--border);border-radius:7px">
          <button class="btn" id="saveCameraIp">Set camera IP</button>
        </div>
        <button class="btn" id="reconnectCam">↻ Reconnect stream</button>
      </div>
    </article>
  </section>
  <section class="panel sensor-section" id="sensors"><div class="panel-head"><div><div class="eyebrow">03 / Live telemetry</div><div class="panel-title">Sensor Parameters</div><div class="panel-desc">Live readings from the main ESP32 controller.</div></div><span class="panel-tag" id="telemetryTag">WAITING FOR DATA</span></div>
    <div class="sensor-grid">
      <div class="sensor"><div class="sensor-top"><span class="sensor-name">Temperature</span><span class="sensor-icon">♨</span></div><div class="sensor-value"><span id="temp">--</span><span style="font-size:12px;color:var(--muted)"> °C</span></div><div class="sensor-unit">DHT11 · ambient temperature</div><div class="mini-bar"><span id="tempBar"></span></div><div class="sensor-note" id="tempNote">Waiting for sensor</div></div>
      <div class="sensor"><div class="sensor-top"><span class="sensor-name">Humidity</span><span class="sensor-icon">◌</span></div><div class="sensor-value"><span id="humidity">--</span><span style="font-size:12px;color:var(--muted)"> %</span></div><div class="sensor-unit">DHT11 · relative humidity</div><div class="mini-bar"><span id="humidityBar"></span></div><div class="sensor-note" id="humidityNote">Waiting for sensor</div></div>
      <div class="sensor"><div class="sensor-top"><span class="sensor-name">Vibration</span><span class="sensor-icon">⌁</span></div><div class="sensor-value"><span id="vibration">--</span></div><div class="sensor-unit">SD30 · ADC counts (0–4095)</div><div class="mini-bar"><span id="vibrationBar"></span></div><div class="sensor-note" id="vibrationNote">Waiting for sensor</div></div>
      <div class="sensor"><div class="sensor-top"><span class="sensor-name">Acoustic input</span><span class="sensor-icon">♫</span></div><div class="sensor-value" id="acoustic">--</div><div class="sensor-unit">ADC counts · GPIO 34</div><div class="mini-bar"><span id="acousticBar"></span></div><div class="sensor-note">Raw signal only · not a confirmed leak</div></div>
    </div>
    <div class="alert-strip"><span class="alert-icon">✓</span><div><strong id="inspectionStatus">Monitoring initialized.</strong><br><span id="inspectionMessage">Automated crack, corrosion and leak classification must be separately trained and validated before this dashboard can report confirmed defects.</span></div></div>
  </section>
  <div class="bottom-line"><span>PIPEGUARD · LOCAL ROBOT CONTROL SYSTEM</span><span id="lastUpdate">Last update: —</span><span>Firmware API · /api/status</span></div>
</main>
</div>
<script>
const API='/api/status';
let CAM_URL = localStorage.getItem('pipeguardCamUrl') || '';
const cameraIpInput = document.getElementById('cameraIpInput');
if (CAM_URL) {
  try { cameraIpInput.value = new URL(CAM_URL).hostname; } catch (e) {}
}
const navLinks = [...document.querySelectorAll('.nav a[data-view]')];
const overviewCards = document.getElementById('overviewCards');
const controlCameraGrid = document.getElementById('controlCameraGrid');
const controlPanel = document.getElementById('control');
const cameraPanel = document.getElementById('camera');
const sensorsPanel = document.getElementById('sensors');

function showView(view) {
  navLinks.forEach(link => {
    const active = link.dataset.view === view;
    link.classList.toggle('active', active);
    if (active) link.setAttribute('aria-current', 'page');
    else link.removeAttribute('aria-current');
  });

  overviewCards.classList.toggle('view-hidden', view !== 'overview');
  controlCameraGrid.classList.toggle('view-hidden', !['control', 'camera'].includes(view));
  controlPanel.classList.toggle('view-hidden', view !== 'control');
  cameraPanel.classList.toggle('view-hidden', view !== 'camera');
  controlCameraGrid.classList.toggle('single-view', ['control', 'camera'].includes(view));
  if (view === 'camera') setTimeout(connectCamera, 150);
  sensorsPanel.classList.toggle('view-hidden', view !== 'sensors');

  const headings = {
    overview: ['Autonomous inspection console', 'Pipeline Operations', 'Monitor robot status and key inspection readings.'],
    control: ['01 / Mobility', 'Robot Control', 'Control robot movement using the dashboard buttons.'],
    camera: ['02 / Visual inspection', 'Live Camera', 'View the pipeline surface through the ESP32-CAM stream.'],
    sensors: ['03 / Live telemetry', 'Sensor Telemetry', 'Monitor live readings from the connected sensors.']
  };
  const [eyebrow, title, subtitle] = headings[view] || headings.overview;
  document.querySelector('.topbar .eyebrow').textContent = eyebrow;
  document.querySelector('.topbar h1').textContent = title;
  document.querySelector('.topbar .subtitle').textContent = subtitle;
  history.replaceState(null, '', '#' + view);
}
navLinks.forEach(link => link.addEventListener('click', event => {
  event.preventDefault();
  showView(link.dataset.view);
}));
showView(location.hash.replace('#', '') || 'overview');

const $=id=>document.getElementById(id);
const dashboardIpEl = document.getElementById('dashboardIp');
if (dashboardIpEl) dashboardIpEl.textContent = location.host || 'not connected';
let cameraConnected=false;
let activeCommand=null;
let keepAliveTimer=null;
let speedSendTimer=null;
const webSpeed=document.getElementById('webSpeed');
const webSpeedValue=document.getElementById('webSpeedValue');
const speedMessage=document.getElementById('speedMessage');
webSpeed.addEventListener('input',()=>{
 webSpeedValue.textContent=webSpeed.value+' / 255';
 $('speedVal').textContent=webSpeed.value;
 $('speedBar').style.width=(Number(webSpeed.value)/255*100)+'%';
 $('speedPercent').textContent=Math.round(Number(webSpeed.value)/255*100)+'%';
 speedMessage.textContent='Applying speed…';
 clearTimeout(speedSendTimer);
 speedSendTimer=setTimeout(()=>{
  fetch('/api/speed?value='+encodeURIComponent(webSpeed.value),{cache:'no-store'})
   .then(r=>r.json())
   .then(d=>{if(!d.ok)throw Error('Speed update failed');speedMessage.textContent='Motor speed set to '+d.speed+' / 255';})
   .catch(()=>{speedMessage.textContent='Speed control unavailable — check ESP32 firmware.';});
 },120);
});

function setRobotState(s){
 $('kpiRobot').textContent=s;
 $('robotState').textContent=s;
}
function sendCommand(cmd){
 return fetch('/api/control?cmd='+encodeURIComponent(cmd),{cache:'no-store'})
  .then(r=>{if(!r.ok)throw Error('HTTP '+r.status);return r.json();})
  .then(d=>setRobotState(d.state||cmd))
  .catch(()=>{$('connectionText').textContent='COMMAND FAILED';});
}
function stopKeepAlive(){
 if(keepAliveTimer!==null){clearInterval(keepAliveTimer);keepAliveTimer=null;}
}
function releaseMovement(){
 if(activeCommand!==null){
  activeCommand=null;
  stopKeepAlive();
  sendCommand('STOP');
 }
}
function startMovement(cmd){
 if(activeCommand===cmd)return;
 activeCommand=cmd;
 stopKeepAlive();
 sendCommand(cmd);
 keepAliveTimer=setInterval(()=>{
  if(activeCommand===cmd)sendCommand(cmd);
 },700);
}
document.querySelectorAll('[data-cmd]').forEach(b=>{
 const cmd=b.dataset.cmd;
 if(cmd==='STOP'){
  b.addEventListener('click',e=>{e.preventDefault();releaseMovement();sendCommand('STOP');});
  return;
 }
 b.addEventListener('pointerdown',e=>{
  e.preventDefault();
  if(b.setPointerCapture){try{b.setPointerCapture(e.pointerId);}catch(err){}}
  startMovement(cmd);
 });
 ['pointerup','pointercancel','pointerleave','lostpointercapture'].forEach(ev=>
  b.addEventListener(ev,()=>releaseMovement())
 );
});
window.addEventListener('pointerup',releaseMovement);
window.addEventListener('blur',releaseMovement);
document.addEventListener('visibilitychange',()=>{if(document.hidden)releaseMovement();});

function saveCameraIp(){
 const raw = cameraIpInput.value.trim().replace(/^https?:\/\//i,'').replace(/\/$/,'');
 if(!raw){$('camMessage').textContent='Enter the camera IP shown in the ESP32-CAM Serial Monitor.';return;}
 const host = raw.replace(/:81$/,'');
 CAM_URL = 'http://' + host + ':81/stream';
 localStorage.setItem('pipeguardCamUrl', CAM_URL);
 $('camMessage').textContent='Camera stream set to ' + CAM_URL;
 connectCamera();
}
function connectCamera(){
 if(!CAM_URL){
   $('camTag').textContent='CAMERA IP NEEDED';
   $('liveLabel').textContent='NO SIGNAL';
   $('camMessage').textContent='Enter the ESP32-CAM IP from its Serial Monitor, then select Set camera IP.';
   $('camPlaceholder').hidden=false;
   $('cameraFeed').hidden=true;
   return;
 }
 const img=$('cameraFeed');
 $('camTag').textContent='CONNECTING';
 $('camMessage').textContent='Attempting to connect to '+CAM_URL+'…';
 $('camPlaceholder').hidden=true;
 img.hidden=false;
 img.onerror=()=>{img.hidden=true;$('camPlaceholder').hidden=false;$('camTag').textContent='CAMERA OFFLINE';$('liveLabel').textContent='NO SIGNAL';$('camMessage').textContent='Cannot load '+CAM_URL+'. Check the camera IP, both boards on the same hotspot, and the stream server.';cameraConnected=false;};
 img.onload=()=>{cameraConnected=true;$('camTag').textContent='STREAM ACTIVE';$('liveLabel').textContent='LIVE';};
 img.src=CAM_URL+'?t='+Date.now();
}
$('saveCameraIp').addEventListener('click',saveCameraIp);
$('connectCam').addEventListener('click',connectCamera);
$('reconnectCam').addEventListener('click',connectCamera);
async function refresh(){
 try{
  const r=await fetch(API,{cache:'no-store'});if(!r.ok)throw Error('HTTP '+r.status);const d=await r.json();
  $('connectionText').textContent='ROBOT CONNECTED';$('sideStatus').textContent='ESP32 connected';
  $('topDot').style.background='var(--green)';$('sideDot').style.background='var(--green)';
  $('lastSeen').textContent='Receiving telemetry';$('lastUpdate').textContent='Last update: '+new Date().toLocaleTimeString();
  setRobotState(d.state||'STOPPED');
  const t=d.temperature===null||d.temperature===undefined?null:Number(d.temperature);
  const h=d.humidity===null||d.humidity===undefined?null:Number(d.humidity);
  const v=d.vibration===null||d.vibration===undefined?null:Number(d.vibration);
  const a=Number(d.acoustic??0),sp=Number(d.potSpeed??d.speed??0);
  $('temp').textContent=t===null?'--':t.toFixed(1);$('kpiTemp').textContent=t===null?'--':t.toFixed(1);
  $('humidity').textContent=h===null?'--':h.toFixed(0);
  $('vibration').textContent=v===null?'--':Math.round(v);$('kpiVib').textContent=v===null?'--':Math.round(v);
  $('acoustic').textContent=a;$('kpiAcoustic').textContent=a;
  $('tempBar').style.width=t===null?'0%':Math.max(0,Math.min(100,t/60*100))+'%';
  $('humidityBar').style.width=h===null?'0%':Math.max(0,Math.min(100,h))+'%';
  $('vibrationBar').style.width=v===null?'0%':Math.max(0,Math.min(100,v/20*100))+'%';
  $('acousticBar').style.width=Math.max(0,Math.min(100,a/4095*100))+'%';
  $('tempNote').textContent=t===null?'DHT11 reading unavailable':'Live sensor reading';
  $('humidityNote').textContent=h===null?'DHT11 reading unavailable':'Live relative humidity';
  $('vibrationNote').textContent=d.vibrationSensor?'SD30 analog reading · GPIO 34':'SD30 unavailable';
  $('speedVal').textContent=sp;$('speedBar').style.width=Math.max(0,Math.min(100,sp/255*100))+'%';$('speedPercent').textContent=Math.round(sp/255*100)+'%';$('webSpeedValue').textContent=sp+' / 255';if(document.activeElement!==$('webSpeed'))$('webSpeed').value=sp;
  $('telemetryTag').textContent=(d.vibrationSensor&&t!==null&&h!==null)?'TELEMETRY LIVE':'PARTIAL TELEMETRY';
  $('inspectionStatus').textContent='Sensor monitoring active.';
  $('inspectionMessage').textContent='Temperature and humidity are from the DHT11. Vibration is the SD30 raw analog ADC reading, not calibrated acceleration. Acoustic sensor and MPU6050 are not connected; these values do not independently confirm a pipeline defect.';
 }catch(e){
  $('connectionText').textContent='ROBOT OFFLINE';$('sideStatus').textContent='ESP32 unavailable';
  $('topDot').style.background='var(--red)';$('sideDot').style.background='var(--red)';
  $('lastSeen').textContent='No telemetry received';$('telemetryTag').textContent='NO DATA';
 }
}
$('refreshBtn').addEventListener('click',refresh);
setInterval(refresh,1000);refresh();
window.addEventListener('blur',()=>sendCommand('STOP'));
window.addEventListener('beforeunload',()=>{navigator.sendBeacon('/api/control?cmd=STOP');});
</script>
</body>
</html>
)PGPAGE";

void handleRoot() {
  server.send_P(200, "text/html", DASHBOARD_HTML);
}

void handleControl() {
  if (!server.hasArg("cmd")) {
    server.send(400, "application/json", "{\"error\":\"missing cmd\"}");
    return;
  }
  String cmd = server.arg("cmd");
  cmd.toUpperCase();
  if (cmd != "FORWARD" && cmd != "REVERSE" && cmd != "LEFT" &&
      cmd != "RIGHT" && cmd != "STOP") {
    server.send(400, "application/json", "{\"error\":\"invalid cmd\"}");
    return;
  }
  setMotion(cmd);
  String json = "{\"ok\":true,\"state\":\"" + robotState + "\"}";
  server.send(200, "application/json", json);
}

void handleSpeed() {
  if (!server.hasArg("value")) {
    server.send(400, "application/json", "{\"ok\":false,\"error\":\"missing value\"}");
    return;
  }
  motorSpeed = constrain(server.arg("value").toInt(), 0, 255);
  if (motorSpeed == 0 && motionActive) stopMotors();
  else if (motionActive) {
    analogWrite(ENA, motorSpeed);
    analogWrite(ENB, motorSpeed);
  }
  Serial.printf("[SPEED] %d\n", motorSpeed);
  server.send(200, "application/json", String("{\"ok\":true,\"speed\":") + motorSpeed + "}");
}

void handleStatus() {
  bool dhtOK = !isnan(temperature) && !isnan(humidity);

  vibrationValue = analogRead(VIBRATION_PIN);

  String json = "{";
  json += "\"state\":\"" + robotState + "\",";
  json += "\"speed\":" + String(motorSpeed) + ",";
  json += "\"potSpeed\":" + String(motorSpeed) + ",";
  json += "\"temperature\":" + String(dhtOK ? String(temperature, 1) : String("null")) + ",";
  json += "\"humidity\":" + String(dhtOK ? String(humidity, 1) : String("null")) + ",";
  json += "\"vibration\":" + String(vibrationValue) + ",";
  json += "\"vibrationSensor\":true,";
  json += "\"acoustic\":null,\"mpu\":false";
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT); pinMode(ENA, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT); pinMode(ENB, OUTPUT);

  pinMode(VIBRATION_PIN, INPUT);
  analogReadResolution(12);

  stopMotors();
  dht.begin();
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("[WIFI] Connecting to mobile hotspot: ");
  Serial.println(WIFI_SSID);
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 30000) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WIFI] Connected to mobile hotspot");
    Serial.print("[WIFI] Main ESP32 dashboard IP: http://");
    Serial.println(WiFi.localIP());
    Serial.print("[WIFI] Gateway: ");
    Serial.println(WiFi.gatewayIP());
  } else {
    Serial.println("[WIFI] FAILED to connect. Check hotspot SSID/password and use 2.4 GHz hotspot.");
  }
  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/control", HTTP_GET, handleControl);
  server.on("/api/speed", HTTP_GET, handleSpeed);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.begin();
  Serial.print("[WEB] Dashboard server started at http://"); Serial.println(WiFi.localIP());
}

void loop() {
  server.handleClient();
  if (motionActive && millis() - lastCommandMs > SAFETY_TIMEOUT_MS) {
    Serial.println("[SAFETY] Command timeout; stopping motors");
    stopMotors();
  }
  if (millis() - lastDhtMs >= 2000) {
    lastDhtMs = millis();
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) { temperature = t; humidity = h; }
  }
}
