/*
 * ================================================================
 *  Micromouse Ground Station — ESP32-C3
 *
 *  What this does:
 *    1. Hosts a Wi-Fi Access Point (no router needed)
 *    2. Serves a web UI at http://192.168.4.1
 *    3. When you submit values from the web UI, they are
 *       packed into a RobotParams struct and broadcast over
 *       ESP-NOW to the robot (ESP32).
 *
 *  How to use:
 *    1. Flash this to the ESP-C3.
 *    2. On your phone/laptop, connect to Wi-Fi:
 *         SSID: "Micromouse-GS"
 *         Pass: "micromouse"
 *    3. Open browser → http://192.168.4.1
 *    4. Edit parameters and click "Send to Robot".
 *    5. The robot's built-in LED flashes to confirm receipt.
 *
 *  Before first use:
 *    • Set ROBOT_MAC_ADDR to the robot ESP32's MAC address.
 *      (It prints on Serial at boot: "[COMMS] ESP-NOW ready. MAC: XX:XX:XX:XX:XX:XX")
 * ================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "robot_params.h"

// ================================================================
//  CONFIG — set these before flashing
// ================================================================

// Robot ESP32 MAC address (get from robot Serial output at boot)
// Example: {0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45}
uint8_t ROBOT_MAC_ADDR[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
//                           ^^^ Replace with actual robot MAC! ^^^
//                           0xFF,0xFF,...,0xFF = broadcast (works but less reliable)

// Wi-Fi Access Point credentials
const char* AP_SSID = "Micromouse-GS";
const char* AP_PASS = "micromouse";

// ================================================================
//  Globals
// ================================================================
AsyncWebServer server(80);
RobotParams currentParams = defaultParams();

// ================================================================
//  ESP-NOW send callback
// ================================================================
void onDataSent(const uint8_t* mac, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        Serial.println("[GS] ESP-NOW: Delivery confirmed");
    } else {
        Serial.println("[GS] ESP-NOW: Delivery FAILED — is robot powered on?");
    }
}

// ================================================================
//  sendParams — pack and broadcast via ESP-NOW
// ================================================================
void sendParams() {
    esp_err_t result = esp_now_send(ROBOT_MAC_ADDR,
                                    (uint8_t*)&currentParams,
                                    sizeof(RobotParams));
    if (result == ESP_OK) {
        Serial.println("[GS] Params sent over ESP-NOW");
    } else {
        Serial.printf("[GS] esp_now_send error: %d\n", result);
    }
}

// ================================================================
//  HTML Web UI (served from memory — no SPIFFS needed)
// ================================================================
const char HTML_PAGE[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Micromouse Ground Station</title>
<style>
  @import url('https://fonts.googleapis.com/css2?family=Inter:wght@400;600;700&display=swap');
  :root{
    --bg:#0d0f1a;--card:#161b2e;--border:#1f2b4a;
    --accent:#4f8ef7;--accent2:#7c5cbf;--green:#2ecc71;
    --text:#e0e6f7;--sub:#7a8aaa;--danger:#e74c3c;
    --radius:12px;--shadow:0 4px 24px rgba(0,0,0,.4);
  }
  *{box-sizing:border-box;margin:0;padding:0}
  body{font-family:'Inter',sans-serif;background:var(--bg);color:var(--text);min-height:100vh;padding:16px}
  h1{text-align:center;font-size:1.6rem;font-weight:700;
     background:linear-gradient(135deg,var(--accent),var(--accent2));
     -webkit-background-clip:text;-webkit-text-fill-color:transparent;
     margin-bottom:4px}
  .subtitle{text-align:center;color:var(--sub);font-size:.85rem;margin-bottom:24px}
  .grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:16px;max-width:1100px;margin:0 auto}
  .card{background:var(--card);border:1px solid var(--border);border-radius:var(--radius);
        padding:20px;box-shadow:var(--shadow)}
  .card h2{font-size:.95rem;font-weight:600;color:var(--accent);margin-bottom:16px;
            text-transform:uppercase;letter-spacing:.08em;border-bottom:1px solid var(--border);padding-bottom:8px}
  .row{display:flex;align-items:center;justify-content:space-between;margin-bottom:12px}
  .row label{font-size:.82rem;color:var(--sub);flex:1;padding-right:8px}
  .row input{width:110px;background:#0a0c18;border:1px solid var(--border);border-radius:8px;
             color:var(--text);padding:6px 10px;font-size:.88rem;text-align:right;
             transition:border .2s}
  .row input:focus{outline:none;border-color:var(--accent)}
  .hint{font-size:.7rem;color:var(--sub);margin-top:-8px;margin-bottom:10px;padding-left:0}
  .btn-row{max-width:1100px;margin:24px auto 0;display:flex;gap:12px;flex-wrap:wrap}
  button{flex:1;padding:14px;border:none;border-radius:var(--radius);font-size:1rem;
         font-weight:700;cursor:pointer;transition:all .2s;letter-spacing:.04em}
  #btnSend{background:linear-gradient(135deg,var(--accent),var(--accent2));color:#fff}
  #btnSend:hover{opacity:.88;transform:translateY(-1px)}
  #btnReset{background:var(--card);border:1px solid var(--border);color:var(--sub)}
  #btnReset:hover{border-color:var(--accent);color:var(--text)}
  #status{max-width:1100px;margin:14px auto 0;padding:12px 16px;border-radius:var(--radius);
          font-size:.88rem;text-align:center;display:none}
  #status.ok{background:#1a3a28;border:1px solid var(--green);color:var(--green)}
  #status.err{background:#3a1a1a;border:1px solid var(--danger);color:var(--danger)}
  .tag{display:inline-block;font-size:.7rem;background:rgba(79,142,247,.15);
       color:var(--accent);border-radius:4px;padding:1px 6px;margin-left:6px;vertical-align:middle}
</style>
</head>
<body>
<h1>&#129302; Micromouse Ground Station</h1>
<p class="subtitle">Edit parameters below and click <strong>Send to Robot</strong> to push via ESP-NOW</p>

<form id="paramForm">
<div class="grid">

  <!-- PID: Left Motor -->
  <div class="card">
    <h2>PID — Left Motor <span class="tag">Motor A</span></h2>
    <div class="row"><label>Kp (Proportional)</label><input type="number" id="pid_L_kp" step="0.01" value="2.00"></div>
    <div class="row"><label>Ki (Integral)</label>    <input type="number" id="pid_L_ki" step="0.001" value="0.000"></div>
    <div class="row"><label>Kd (Derivative)</label>  <input type="number" id="pid_L_kd" step="0.01" value="0.50"></div>
  </div>

  <!-- PID: Right Motor -->
  <div class="card">
    <h2>PID — Right Motor <span class="tag">Motor B</span></h2>
    <div class="row"><label>Kp (Proportional)</label><input type="number" id="pid_R_kp" step="0.01" value="2.00"></div>
    <div class="row"><label>Ki (Integral)</label>    <input type="number" id="pid_R_ki" step="0.001" value="0.000"></div>
    <div class="row"><label>Kd (Derivative)</label>  <input type="number" id="pid_R_kd" step="0.01" value="0.50"></div>
  </div>

  <!-- PID: Wall Following -->
  <div class="card">
    <h2>PID — Wall Following</h2>
    <div class="row"><label>Kp (Proportional)</label><input type="number" id="pid_W_kp" step="0.01" value="1.50"></div>
    <div class="row"><label>Ki (Integral)</label>    <input type="number" id="pid_W_ki" step="0.001" value="0.000"></div>
    <div class="row"><label>Kd (Derivative)</label>  <input type="number" id="pid_W_kd" step="0.01" value="0.30"></div>
  </div>

  <!-- Motor Speeds -->
  <div class="card">
    <h2>Motor Speeds <span class="tag">0–1023</span></h2>
    <div class="row"><label>Forward speed</label>  <input type="number" id="speed_fwd"  min="0" max="1023" value="400"></div>
    <div class="row"><label>Turn speed</label>     <input type="number" id="speed_turn" min="0" max="1023" value="350"></div>
    <div class="row"><label>Slow / approach</label><input type="number" id="speed_slow" min="0" max="1023" value="200"></div>
  </div>

  <!-- Maze / Navigation -->
  <div class="card">
    <h2>Maze Navigation</h2>
    <div class="row"><label>Cell size (mm)</label>        <input type="number" id="cell_size_mm" value="180"></div>
    <div class="row"><label>Forward ticks (1 cell)</label><input type="number" id="fwd_ticks"    value="400"></div>
    <div class="row"><label>Turn ticks (90°)</label>      <input type="number" id="turn_ticks"   value="220"></div>
  </div>

  <!-- Sensor Thresholds -->
  <div class="card">
    <h2>Sensor Thresholds <span class="tag">mm</span></h2>
    <div class="row"><label>Front wall detect (mm)</label>   <input type="number" id="wall_front_thresh" value="80"></div>
    <div class="row"><label>Side wall detect (mm)</label>    <input type="number" id="wall_side_thresh"  value="90"></div>
    <div class="row"><label>Diagonal threshold (mm)</label>  <input type="number" id="wall_diag_thresh"  value="120"></div>
  </div>

  <!-- Calibration -->
  <div class="card">
    <h2>Calibration</h2>
    <div class="row"><label>Left encoder direction (+1 or -1)</label> <input type="number" id="enc_L_direction" value="1" min="-1" max="1"></div>
    <div class="row"><label>Right encoder direction (+1 or -1)</label><input type="number" id="enc_R_direction" value="1" min="-1" max="1"></div>
    <div class="row"><label>Sensor read delay (µs)</label>            <input type="number" id="sensor_read_us" value="20" min="10" max="100"></div>
  </div>

</div><!-- /grid -->
</form>

<div class="btn-row">
  <button id="btnReset" type="button" onclick="resetDefaults()">&#8635; Reset Defaults</button>
  <button id="btnSend"  type="button" onclick="sendParams()">&#128225; Send to Robot</button>
</div>
<div id="status"></div>

<script>
const defaults = {
  pid_L_kp:2.0, pid_L_ki:0.0, pid_L_kd:0.5,
  pid_R_kp:2.0, pid_R_ki:0.0, pid_R_kd:0.5,
  pid_W_kp:1.5, pid_W_ki:0.0, pid_W_kd:0.3,
  speed_fwd:400, speed_turn:350, speed_slow:200,
  cell_size_mm:180, fwd_ticks:400, turn_ticks:220,
  wall_front_thresh:80, wall_side_thresh:90, wall_diag_thresh:120,
  enc_L_direction:1, enc_R_direction:1, sensor_read_us:20
};

function resetDefaults(){
  for(const [k,v] of Object.entries(defaults)){
    const el=document.getElementById(k);
    if(el) el.value=v;
  }
  showStatus('Defaults restored (not yet sent to robot)', false);
}

function showStatus(msg, ok){
  const el=document.getElementById('status');
  el.textContent=msg;
  el.className=ok?'ok':'err';
  el.style.display='block';
  setTimeout(()=>{el.style.display='none'},4000);
}

async function sendParams(){
  const ids=[
    'pid_L_kp','pid_L_ki','pid_L_kd',
    'pid_R_kp','pid_R_ki','pid_R_kd',
    'pid_W_kp','pid_W_ki','pid_W_kd',
    'speed_fwd','speed_turn','speed_slow',
    'cell_size_mm','fwd_ticks','turn_ticks',
    'wall_front_thresh','wall_side_thresh','wall_diag_thresh',
    'enc_L_direction','enc_R_direction','sensor_read_us'
  ];
  const params={};
  for(const id of ids){
    const el=document.getElementById(id);
    params[id]=parseFloat(el.value);
  }
  try{
    const resp=await fetch('/send',{
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body:JSON.stringify(params)
    });
    if(resp.ok){
      showStatus('✓ Sent to robot via ESP-NOW', true);
    } else {
      showStatus('✗ Server error: '+resp.status, false);
    }
  } catch(e){
    showStatus('✗ Network error: '+e.message, false);
  }
}
</script>
</body>
</html>
)rawhtml";

// ================================================================
//  setup()
// ================================================================
void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("\n=== Micromouse Ground Station (ESP-C3) ===");

    // --- Start Wi-Fi AP ---
    WiFi.mode(WIFI_AP_STA);   // AP + STA needed for ESP-NOW over AP
    WiFi.softAP(AP_SSID, AP_PASS);
    Serial.printf("[GS] AP started: SSID='%s'  IP=%s\n",
                  AP_SSID, WiFi.softAPIP().toString().c_str());
    Serial.println("[GS] Connect to this Wi-Fi, then open http://192.168.4.1");

    // --- Init ESP-NOW ---
    if (esp_now_init() != ESP_OK) {
        Serial.println("[GS] ERROR: ESP-NOW init failed!");
        return;
    }
    esp_now_register_send_cb(onDataSent);

    // Register robot as peer
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, ROBOT_MAC_ADDR, 6);
    peer.channel = 0;
    peer.encrypt = false;
    if (esp_now_add_peer(&peer) != ESP_OK) {
        Serial.println("[GS] WARN: Failed to add peer — check ROBOT_MAC_ADDR");
    } else {
        Serial.println("[GS] Robot peer registered");
    }

    // --- Web Server routes ---

    // Serve main page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send_P(200, "text/html", HTML_PAGE);
    });

    // Handle parameter POST from web UI
    server.on("/send", HTTP_POST,
        [](AsyncWebServerRequest* req) {},   // empty request handler
        NULL,
        [](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
            // Parse JSON body
            DynamicJsonDocument doc(1024);
            DeserializationError err = deserializeJson(doc, data, len);
            if (err) {
                req->send(400, "text/plain", "Bad JSON");
                return;
            }

            // Map JSON fields → RobotParams struct
            currentParams.pid_L_kp = doc["pid_L_kp"] | currentParams.pid_L_kp;
            currentParams.pid_L_ki = doc["pid_L_ki"] | currentParams.pid_L_ki;
            currentParams.pid_L_kd = doc["pid_L_kd"] | currentParams.pid_L_kd;
            currentParams.pid_R_kp = doc["pid_R_kp"] | currentParams.pid_R_kp;
            currentParams.pid_R_ki = doc["pid_R_ki"] | currentParams.pid_R_ki;
            currentParams.pid_R_kd = doc["pid_R_kd"] | currentParams.pid_R_kd;
            currentParams.pid_W_kp = doc["pid_W_kp"] | currentParams.pid_W_kp;
            currentParams.pid_W_ki = doc["pid_W_ki"] | currentParams.pid_W_ki;
            currentParams.pid_W_kd = doc["pid_W_kd"] | currentParams.pid_W_kd;
            currentParams.speed_fwd    = doc["speed_fwd"]    | currentParams.speed_fwd;
            currentParams.speed_turn   = doc["speed_turn"]   | currentParams.speed_turn;
            currentParams.speed_slow   = doc["speed_slow"]   | currentParams.speed_slow;
            currentParams.cell_size_mm = doc["cell_size_mm"] | currentParams.cell_size_mm;
            currentParams.fwd_ticks    = doc["fwd_ticks"]    | currentParams.fwd_ticks;
            currentParams.turn_ticks   = doc["turn_ticks"]   | currentParams.turn_ticks;
            currentParams.wall_front_thresh = doc["wall_front_thresh"] | currentParams.wall_front_thresh;
            currentParams.wall_side_thresh  = doc["wall_side_thresh"]  | currentParams.wall_side_thresh;
            currentParams.wall_diag_thresh  = doc["wall_diag_thresh"]  | currentParams.wall_diag_thresh;
            currentParams.enc_L_direction   = doc["enc_L_direction"]   | currentParams.enc_L_direction;
            currentParams.enc_R_direction   = doc["enc_R_direction"]   | currentParams.enc_R_direction;
            currentParams.sensor_read_us    = doc["sensor_read_us"]    | currentParams.sensor_read_us;

            sendParams();
            req->send(200, "text/plain", "OK");

            Serial.println("[GS] Web UI submitted new params:");
            Serial.printf("  PID_L: kp=%.3f ki=%.3f kd=%.3f\n",
                          currentParams.pid_L_kp, currentParams.pid_L_ki, currentParams.pid_L_kd);
        }
    );

    server.begin();
    Serial.println("[GS] Web server started at http://192.168.4.1");
}

// ================================================================
//  loop() — nothing to poll; AsyncWebServer + ESP-NOW use callbacks
// ================================================================
void loop() {
    delay(1000);
}
