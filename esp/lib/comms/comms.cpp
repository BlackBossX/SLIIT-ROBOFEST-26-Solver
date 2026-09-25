#include "comms.h"

// =========================================================
//  Live parameter set — starts with defaults, overwritten
//  when Ground Station sends an ESP-NOW update.
// =========================================================
static RobotParams liveParams = defaultParams();
static volatile bool newParamsFlag = false;

// =========================================================
//  ESP-NOW receive callback
//  Called automatically when a packet arrives.
// =========================================================
static void onDataReceive(const uint8_t* mac, const uint8_t* data, int len) {
    if (len == sizeof(RobotParams)) {
        memcpy(&liveParams, data, sizeof(RobotParams));
        newParamsFlag = true;
        // Brief LED flash to confirm receipt
        digitalWrite(2, HIGH); delay(50); digitalWrite(2, LOW);
        Serial.println("[COMMS] New params received from Ground Station");
    } else {
        Serial.printf("[COMMS] WARN: Received %d bytes, expected %d — ignored\n",
                      len, (int)sizeof(RobotParams));
    }
}

// =========================================================
//  Comms_Init — start Wi-Fi in STA mode and init ESP-NOW
// =========================================================
void Comms_Init(void) {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();      // Don't connect to any AP

    if (esp_now_init() != ESP_OK) {
        Serial.println("[COMMS] ERROR: ESP-NOW init failed!");
        return;
    }
    esp_now_register_recv_cb(onDataReceive);

    Serial.printf("[COMMS] ESP-NOW ready. MAC: %s\n",
                  WiFi.macAddress().c_str());
    Serial.println("[COMMS] Waiting for Ground Station updates...");
}

// =========================================================
//  paramsUpdated — returns true once per new update
// =========================================================
bool paramsUpdated(void) {
    if (newParamsFlag) {
        newParamsFlag = false;
        return true;
    }
    return false;
}

// =========================================================
//  getParams — reference to live parameters
// =========================================================
RobotParams& getParams(void) {
    return liveParams;
}

// =========================================================
//  printParams — dump all values to Serial for debugging
// =========================================================
void printParams(void) {
    const RobotParams& p = liveParams;
    Serial.println("======= Robot Parameters =======");
    Serial.printf("  PID Left   Kp=%.3f Ki=%.3f Kd=%.3f\n", p.pid_L_kp, p.pid_L_ki, p.pid_L_kd);
    Serial.printf("  PID Right  Kp=%.3f Ki=%.3f Kd=%.3f\n", p.pid_R_kp, p.pid_R_ki, p.pid_R_kd);
    Serial.printf("  PID Wall   Kp=%.3f Ki=%.3f Kd=%.3f\n", p.pid_W_kp, p.pid_W_ki, p.pid_W_kd);
    Serial.printf("  Speed  fwd=%d turn=%d slow=%d\n",       p.speed_fwd, p.speed_turn, p.speed_slow);
    Serial.printf("  Cell   size=%dmm fwd_ticks=%d turn_ticks=%d\n",
                  p.cell_size_mm, p.fwd_ticks, p.turn_ticks);
    Serial.printf("  Wall thresh  front=%dmm side=%dmm diag=%dmm\n",
                  p.wall_front_thresh, p.wall_side_thresh, p.wall_diag_thresh);
    Serial.printf("  Enc direction L=%+d R=%+d\n", p.enc_L_direction, p.enc_R_direction);
    Serial.println("================================");
}
