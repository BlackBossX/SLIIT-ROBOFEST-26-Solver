/*
 * ================================================================
 *  Micromouse Ground Station — ESP32-C3 Super Mini
 *  Fixed for C3 Super Mini: no LR mode, correct init order,
 *  settling delays, stable AP + ESP-NOW coexistence
 * ================================================================
 */

#include <WiFi.h>
#include <WebServer.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <ArduinoJson.h>
#include "robot_params.h"

// ================================================================
//  CONFIG — edit before flashing
// ================================================================

// Robot MAC — read from robot's Serial output at boot
uint8_t ROBOT_MAC_ADDR[6] = {0x44, 0x1D, 0x64, 0xF9, 0x79, 0x70};

const char*    AP_SSID      = "Micromouse-GS";
const char*    AP_PASS      = "micromouse";
const uint8_t  WIFI_CHANNEL = 1;   // try 6 or 11 if still invisible

// ================================================================
//  Globals
// ================================================================
WebServer   server(80);
RobotParams currentParams = defaultParams();

// ================================================================
//  ESP-NOW send callback
// ================================================================
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
void onDataSent(const wifi_action_tx_status_t* status) {
    Serial.println(status->status == ESP_NOW_SEND_SUCCESS
        ? "[GS] ESP-NOW: Delivery confirmed"
        : "[GS] ESP-NOW: Delivery FAILED — is robot on?");
}
#else
void onDataSent(const uint8_t* mac, esp_now_send_status_t status) {
    Serial.println(status == ESP_NOW_SEND_SUCCESS
        ? "[GS] ESP-NOW: Delivery confirmed"
        : "[GS] ESP-NOW: Delivery FAILED — is robot on?");
}
#endif

// ================================================================
//  sendParams — broadcast struct via ESP-NOW
// ================================================================
void sendParams() {
    esp_err_t result = esp_now_send(
        ROBOT_MAC_ADDR,
        (uint8_t*)&currentParams,
        sizeof(RobotParams)
    );
    if (result == ESP_OK) {
        Serial.println("[GS] Params sent over ESP-NOW");
    } else {
        Serial.printf("[GS] esp_now_send error: 0x%x\n", result);
    }
}

// ================================================================
//  HTML Web UI
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
  .card{background:var(--card);border:1px solid var(--border);border-radius:var(--radius);padding:20px;box-shadow:var(--shadow)}
  .card h2{font-size:.95rem;font-weight:600;color:var(--accent);margin-bottom:16px;
            text-transform:uppercase;letter-spacing:.08em;border-bottom:1px solid var(--border);padding-bottom:8px}
  .row{display:flex;align-items:center;justify-content:space-between;margin-bottom:12px}
  .row label{font-size:.82rem;color:var(--sub);flex:1;padding-right:8px}
  .row input{width:110px;background:#0a0c18;border:1px solid var(--border);border-radius:8px;
             color:var(--text);padding:6px 10px;font-size:.88rem;text-align:right;transition:border .2s}
  .row input:focus{outline:none;border-color:var(--accent)}
  .btn-row{max-width:1100px;margin:24px auto 0;display:flex;gap:12px;flex-wrap:wrap}
  button{flex:1;padding:14px;border:none;border-radius:var(--radius);font-size:1rem;
         font-weight:700;cursor:pointer;transition:all .2s;letter-spacing:.04em}
  #btnSend{background:linear-gradient(135deg,var(--accent),var(--accent2));color:#fff}
  #btnSend:hover{opacity:.88;transform:translateY(-1px)}
  #btnReset{background:var(--card);border:1px solid var(--border);color:var(--sub)}
  #btnReset:hover{border-color:var(--accent);color:var(--text)}
  .cmd-btn{background:var(--card);border:1px solid var(--border);color:var(--text);
           padding:10px;font-size:.82rem;border-radius:8px;flex:none;width:calc(50% - 5px)}
  .cmd-btn:hover{border-color:var(--accent);color:var(--accent)}
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
<p class="subtitle">Edit parameters then click <strong>Send to Robot</strong> to push via ESP-NOW</p>

<form id="paramForm">
<div class="grid">

  <div class="card">
    <h2>PID — Motors <span class="tag">Unified</span></h2>
    <div class="row"><label>Kp</label><input type="number" id="pid_M_kp" step="0.01" value="2.00"></div>
    <div class="row"><label>Ki</label><input type="number" id="pid_M_ki" step="0.001" value="0.000"></div>
    <div class="row"><label>Kd</label><input type="number" id="pid_M_kd" step="0.01" value="0.50"></div>
  </div>

  <div class="card">
    <h2>PID — Wall Following</h2>
    <div class="row"><label>Kp</label><input type="number" id="pid_W_kp" step="0.01" value="1.50"></div>
    <div class="row"><label>Ki</label><input type="number" id="pid_W_ki" step="0.001" value="0.000"></div>
    <div class="row"><label>Kd</label><input type="number" id="pid_W_kd" step="0.01" value="0.30"></div>
  </div>

  <div class="card">
    <h2>Motor Speeds <span class="tag">0–1023</span></h2>
    <div class="row"><label>Forward</label>  <input type="number" id="speed_fwd"  min="0" max="1023" value="400"></div>
    <div class="row"><label>Turn</label>     <input type="number" id="speed_turn" min="0" max="1023" value="350"></div>
    <div class="row"><label>Slow</label>     <input type="number" id="speed_slow" min="0" max="1023" value="200"></div>
  </div>

  <div class="card">
    <h2>Maze Navigation</h2>
    <div class="row"><label>Cell size (mm)</label>        <input type="number" id="cell_size_mm" value="180"></div>
    <div class="row"><label>Forward ticks (1 cell)</label><input type="number" id="fwd_ticks"    value="400"></div>
    <div class="row"><label>Turn ticks (90°)</label>      <input type="number" id="turn_ticks"   value="220"></div>
  </div>

  <div class="card">
    <h2>Sensor Thresholds <span class="tag">mm</span></h2>
    <div class="row"><label>Front wall</label>   <input type="number" id="wall_front_thresh" value="80"></div>
    <div class="row"><label>Side wall</label>    <input type="number" id="wall_side_thresh"  value="90"></div>
    <div class="row"><label>Diagonal</label>     <input type="number" id="wall_diag_thresh"  value="120"></div>
  </div>

  <div class="card">
    <h2>Calibration</h2>
    <div class="row"><label>Left encoder dir (+1 / -1)</label> <input type="number" id="enc_L_direction" value="1"  min="-1" max="1"></div>
    <div class="row"><label>Right encoder dir (+1 / -1)</label><input type="number" id="enc_R_direction" value="1"  min="-1" max="1"></div>
    <div class="row"><label>Sensor read delay (µs)</label>     <input type="number" id="sensor_read_us"  value="20" min="10" max="100"></div>
  </div>

  <div class="card">
    <h2>Hardware Tuner</h2>
    <div class="row"><label>Motor L Trim</label>  <input type="number" id="motor_l_trim" step="0.01" value="1.17"></div>
    <div class="row"><label>Motor R Trim</label>  <input type="number" id="motor_r_trim" step="0.01" value="1.00"></div>
    <div class="row"><label>Encoder kp_bal</label><input type="number" id="kp_bal" step="0.01" value="1.42"></div>
    <div class="row"><label>Min PWM</label>       <input type="number" id="min_pwm" min="0" max="1023" value="0"></div>
    <div class="row"><label>Invert Flags</label>  <input type="number" id="invert_flags" min="0" max="255" value="0"></div>
  </div>

  <div class="card">
    <h2>Manual Commands</h2>
    <div style="display:flex;flex-wrap:wrap;gap:10px;margin-top:8px">
      <button class="cmd-btn" type="button" onclick="sendCommand(9)" style="border-color:var(--green);color:var(--green)">▶ Start Auto</button>
      <button class="cmd-btn" type="button" onclick="sendCommand(8)" style="border-color:var(--danger);color:var(--danger)">■ Stop Auto</button>
      <button class="cmd-btn" type="button" onclick="sendCommand(1)">▲ Forward</button>
      <button class="cmd-btn" type="button" onclick="sendCommand(2)">▼ Backward</button>
      <button class="cmd-btn" type="button" onclick="sendCommand(3)">↺ Turn 90° Left</button>
      <button class="cmd-btn" type="button" onclick="sendCommand(4)">↻ Turn 90° Right</button>
      <button class="cmd-btn" type="button" onclick="sendCommand(5)">↺ Turn 45° Left</button>
      <button class="cmd-btn" type="button" onclick="sendCommand(6)">↻ Turn 45° Right</button>
      <button class="cmd-btn" type="button" onclick="sendCommand(7)">↕ Turn 180°</button>
    </div>
  </div>

</div>
</form>

<div class="btn-row">
  <button id="btnReset" type="button" onclick="resetDefaults()">↺ Reset Defaults</button>
  <button id="btnSend"  type="button" onclick="sendParams()">📡 Send to Robot</button>
</div>
<div id="status"></div>

<script>
const defaults = {
  pid_M_kp:2.0, pid_M_ki:0.0, pid_M_kd:0.5,
  pid_W_kp:1.5, pid_W_ki:0.0, pid_W_kd:0.3,
  speed_fwd:400, speed_turn:350, speed_slow:200,
  cell_size_mm:180, fwd_ticks:400, turn_ticks:220,
  wall_front_thresh:80, wall_side_thresh:90, wall_diag_thresh:120,
  enc_L_direction:1, enc_R_direction:1, sensor_read_us:20,
  motor_l_trim:1.17, motor_r_trim:1.0, kp_bal:1.42, min_pwm:0, invert_flags:0
};

function resetDefaults(){
  for(const [k,v] of Object.entries(defaults)){
    const el=document.getElementById(k);
    if(el) el.value=v;
  }
  showStatus('Defaults restored — not yet sent to robot', false);
}

function showStatus(msg, ok){
  const el=document.getElementById('status');
  el.textContent=msg;
  el.className=ok?'ok':'err';
  el.style.display='block';
  clearTimeout(el._t);
  el._t=setTimeout(()=>{el.style.display='none'},4000);
}

async function sendCommand(cmd_id){
  try{
    const resp=await fetch('/send',{
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body:JSON.stringify({command_id:cmd_id})
    });
    showStatus(resp.ok ? '✓ Command sent' : '✗ Server error: '+resp.status, resp.ok);
  } catch(e){
    showStatus('✗ Network error: '+e.message, false);
  }
}

async function sendParams(){
  const ids=[
    'pid_M_kp','pid_M_ki','pid_M_kd',
    'pid_W_kp','pid_W_ki','pid_W_kd',
    'speed_fwd','speed_turn','speed_slow',
    'cell_size_mm','fwd_ticks','turn_ticks',
    'wall_front_thresh','wall_side_thresh','wall_diag_thresh',
    'enc_L_direction','enc_R_direction','sensor_read_us',
    'motor_l_trim','motor_r_trim','kp_bal','min_pwm','invert_flags'
  ];
  const params={};
  for(const id of ids){
    params[id]=parseFloat(document.getElementById(id).value);
  }
  try{
    const resp=await fetch('/send',{
      method:'POST',
      headers:{'Content-Type':'application/json'},
      body:JSON.stringify(params)
    });
    showStatus(resp.ok ? '✓ Sent to robot via ESP-NOW' : '✗ Server error: '+resp.status, resp.ok);
  } catch(e){
    showStatus('✗ Network error: '+e.message, false);
  }
}
</script>
</body>
</html>
)rawhtml";

// ================================================================
//  HTTP Handlers
// ================================================================

void handleRoot() {
    server.send_P(200, "text/html", HTML_PAGE);
}

void handleSend() {
    if (!server.hasArg("plain")) {
        server.send(400, "text/plain", "Body missing");
        return;
    }

    #if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
    #else
    DynamicJsonDocument doc(1024);
    #endif

    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (err) {
        server.send(400, "text/plain", "Bad JSON");
        return;
    }

    // Update only fields present in the JSON (others keep current value)
    currentParams.pid_M_kp = doc["pid_M_kp"] | currentParams.pid_M_kp;
    currentParams.pid_M_ki = doc["pid_M_ki"] | currentParams.pid_M_ki;
    currentParams.pid_M_kd = doc["pid_M_kd"] | currentParams.pid_M_kd;
    currentParams.pid_W_kp = doc["pid_W_kp"] | currentParams.pid_W_kp;
    currentParams.pid_W_ki = doc["pid_W_ki"] | currentParams.pid_W_ki;
    currentParams.pid_W_kd = doc["pid_W_kd"] | currentParams.pid_W_kd;
    currentParams.speed_fwd         = doc["speed_fwd"]         | currentParams.speed_fwd;
    currentParams.speed_turn        = doc["speed_turn"]        | currentParams.speed_turn;
    currentParams.speed_slow        = doc["speed_slow"]        | currentParams.speed_slow;
    currentParams.cell_size_mm      = doc["cell_size_mm"]      | currentParams.cell_size_mm;
    currentParams.fwd_ticks         = doc["fwd_ticks"]         | currentParams.fwd_ticks;
    currentParams.turn_ticks        = doc["turn_ticks"]        | currentParams.turn_ticks;
    currentParams.wall_front_thresh = doc["wall_front_thresh"] | currentParams.wall_front_thresh;
    currentParams.wall_side_thresh  = doc["wall_side_thresh"]  | currentParams.wall_side_thresh;
    currentParams.wall_diag_thresh  = doc["wall_diag_thresh"]  | currentParams.wall_diag_thresh;
    currentParams.enc_L_direction   = doc["enc_L_direction"]   | currentParams.enc_L_direction;
    currentParams.enc_R_direction   = doc["enc_R_direction"]   | currentParams.enc_R_direction;
    currentParams.sensor_read_us    = doc["sensor_read_us"]    | currentParams.sensor_read_us;
    currentParams.motor_l_trim      = doc["motor_l_trim"]      | currentParams.motor_l_trim;
    currentParams.motor_r_trim      = doc["motor_r_trim"]      | currentParams.motor_r_trim;
    currentParams.kp_bal            = doc["kp_bal"]            | currentParams.kp_bal;
    currentParams.min_pwm           = doc["min_pwm"]           | currentParams.min_pwm;
    currentParams.invert_flags      = doc["invert_flags"]      | currentParams.invert_flags;
    currentParams.command_id        = doc["command_id"]        | 0;

    sendParams();
    server.send(200, "text/plain", "OK");

    Serial.printf("[GS] Params sent | cmd=%d | PID_M: %.2f/%.3f/%.2f | spd_fwd: %d\n",
        currentParams.command_id,
        currentParams.pid_M_kp, currentParams.pid_M_ki, currentParams.pid_M_kd,
        currentParams.speed_fwd);
}

// ================================================================
//  setup() — C3 Super Mini safe init order
// ================================================================
void setup() {
    Serial.begin(115200);
    delay(500);   // C3 Super Mini needs longer boot settle
    Serial.println("\n=== Micromouse Ground Station (C3 Super Mini) ===");

    // 1. Set mode first
    WiFi.mode(WIFI_AP_STA);
    delay(100);

    // 2. Disable power saving BEFORE starting AP
    esp_wifi_set_ps(WIFI_PS_NONE);

    // 3. Start AP — NO LR mode, it breaks AP visibility on C3 Super Mini
    WiFi.softAP(AP_SSID, AP_PASS, WIFI_CHANNEL, 0, 4);
    delay(150);   // let AP fully start before configuring IP

    // 4. Lock IP
    WiFi.softAPConfig(
        IPAddress(192, 168, 4, 1),
        IPAddress(192, 168, 4, 1),
        IPAddress(255, 255, 255, 0)
    );

    // 5. Max TX power (after AP start)
    WiFi.setTxPower(WIFI_POWER_15dBm);

    Serial.printf("[GS] AP: SSID='%s'  IP=%s  CH=%d  TXpwr=%d\n",
        AP_SSID,
        WiFi.softAPIP().toString().c_str(),
        WIFI_CHANNEL,
        WiFi.getTxPower());
    Serial.println("[GS] Connect to Wi-Fi, then open http://192.168.4.1");

    // 6. Init ESP-NOW (after Wi-Fi is up)
    if (esp_now_init() != ESP_OK) {
        Serial.println("[GS] ERROR: ESP-NOW init failed! Halting.");
        while (true) delay(1000);
    }
    esp_now_register_send_cb(onDataSent);

    // 7. Register robot peer
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, ROBOT_MAC_ADDR, 6);
    peer.channel = WIFI_CHANNEL;
    peer.encrypt = false;
    if (esp_now_add_peer(&peer) != ESP_OK) {
        Serial.println("[GS] WARN: Failed to add robot peer — check ROBOT_MAC_ADDR");
    } else {
        Serial.println("[GS] Robot peer registered OK");
    }

    // 8. Web server routes
    server.on("/",     HTTP_GET,  handleRoot);
    server.on("/send", HTTP_POST, handleSend);
    server.begin();
    Serial.println("[GS] Web server ready at http://192.168.4.1");
}

// ================================================================
//  loop()
// ================================================================
void loop() {
    server.handleClient();
    delay(2);
}