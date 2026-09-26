#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "driver/pcnt.h"
#include "web_ui.h"

// =============================================================================
// WI-FI CONFIGURATION
// =============================================================================
// The robot creates its own standalone Access Point hotspot.
// No router or ground station is needed!
// Connect your phone/laptop to this Wi-Fi network and open: http://192.168.4.1
const char* AP_SSID = "MicroMouse-Tuner";
const char* AP_PASS = "12345678"; // Min 8 characters, or set to "" for open network

// (Optional) Connect to home/lab Wi-Fi simultaneously if desired:
const char* STA_SSID = ""; // Leave blank "" to run pure Access Point
const char* STA_PASS = "";

// =============================================================================
// PIN DEFINITIONS (TB6612FNG Dual H-Bridge & PCNT Quadrature Encoders)
// =============================================================================
// TB6612FNG Motor Driver
// Left Motor (Motor B):
const int PWM_L  = 17;
const int L_IN1  = 4;
const int L_IN2  = 16;

// Right Motor (Motor A):
const int PWM_R  = 23;
const int R_IN1  = 18;
const int R_IN2  = 19;

const int PWM_FREQ = 20000;
const int PWM_RES  = 8; // 0 - 255

// Hardware Encoders via ESP32 PCNT
// Left Encoder:
#define ENC_L_PULSE_PIN 34
#define ENC_L_CTRL_PIN  36

// Right Encoder:
#define ENC_R_PULSE_PIN 39
#define ENC_R_CTRL_PIN  35

#define PCNT_UNIT_LEFT  PCNT_UNIT_0
#define PCNT_UNIT_RIGHT PCNT_UNIT_1

// Optional I2C OLED Display (0.96" SSD1306)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
bool oledFound = false;

// =============================================================================
// ROBOT TUNING CONFIGURATION & NVS STORAGE
// =============================================================================
struct RobotConfig {
  float ticks_per_mm;   // Calibration ratio: encoder ticks per millimeter
  float motor_l_trim;   // Multiplier for Left Motor (e.g. 1.00)
  float motor_r_trim;   // Multiplier for Right Motor (e.g. 1.12 if right motor is tight)
  uint8_t min_pwm;      // Deadband floor to overcome gearbox stiction
  float kp_bal;         // Straight balance PID Proportional gain
  float ki_bal;         // Straight balance PID Integral gain (KILLS STIFF MOTOR LAG!)
  float kd_bal;         // Straight balance PID Derivative gain
  uint8_t invert_flags; // Bit 0: Inv M_L, Bit 1: Inv M_R, Bit 2: Inv E_L, Bit 3: Inv E_R
};

RobotConfig cfg = {
  .ticks_per_mm = 8.45f,
  .motor_l_trim = 1.00f,
  .motor_r_trim = 1.00f,
  .min_pwm = 45,
  .kp_bal = 0.45f,
  .ki_bal = 0.03f,
  .kd_bal = 0.15f,
  .invert_flags = 0x00
};

Preferences prefs;
WebServer server(80);

// =============================================================================
// RUNTIME STATE & TELEMETRY
// =============================================================================
enum MotionState {
  STATE_IDLE,
  STATE_MOVE_FWD,
  STATE_MOVE_REV,
  STATE_TEST_SOLO_L,
  STATE_TEST_SOLO_R,
  STATE_OPEN_LOOP_TEST,
  STATE_BRAKING
};

volatile MotionState motionState = STATE_IDLE;

int32_t encLeftAcc = 0;
int32_t encRightAcc = 0;
int16_t lastPcntL = 0;
int16_t lastPcntR = 0;

int16_t currentPwmL = 0;
int16_t currentPwmR = 0;
float currentSteer = 0.0f;

// Motion parameters
int32_t startEncL = 0;
int32_t startEncR = 0;
int32_t targetDistTicks = 0;
float targetDistMm = 0.0f;
float distTraveledMm = 0.0f;
int motionBasePwm = 120;
unsigned long motionStartTime = 0;
unsigned long motionDurationMs = 0;
int32_t lastCompletedTraveledTicks = 0;

// Balance PID state
float balIntegral = 0.0f;
float lastBalError = 0.0f;
unsigned long lastBalTime = 0;

// Open-Loop Friction Diagnostic State
bool openLoopCompleted = false;
int32_t openLoopTicksL = 0;
int32_t openLoopTicksR = 0;

// =============================================================================
// PREFERENCES (NVS) STORAGE IN FLASH
// =============================================================================
void loadConfig() {
  prefs.begin("mm_tuner", true);
  cfg.ticks_per_mm = prefs.getFloat("t_per_mm", cfg.ticks_per_mm);
  cfg.motor_l_trim = prefs.getFloat("m_l_trim", cfg.motor_l_trim);
  cfg.motor_r_trim = prefs.getFloat("m_r_trim", cfg.motor_r_trim);
  cfg.min_pwm      = prefs.getUChar("min_pwm",  cfg.min_pwm);
  cfg.kp_bal       = prefs.getFloat("kp_bal",   cfg.kp_bal);
  cfg.ki_bal       = prefs.getFloat("ki_bal",   cfg.ki_bal);
  cfg.kd_bal       = prefs.getFloat("kd_bal",   cfg.kd_bal);
  cfg.invert_flags = prefs.getUChar("inv_flg",  cfg.invert_flags);
  prefs.end();
  Serial.printf("[NVS] Loaded config: Ticks/mm=%.2f, TrimL=%.2f, TrimR=%.2f, MinPWM=%d, Kp=%.2f, Ki=%.3f, Kd=%.2f\n",
                cfg.ticks_per_mm, cfg.motor_l_trim, cfg.motor_r_trim, cfg.min_pwm, cfg.kp_bal, cfg.ki_bal, cfg.kd_bal);
}

void saveConfig() {
  prefs.begin("mm_tuner", false);
  prefs.putFloat("t_per_mm", cfg.ticks_per_mm);
  prefs.putFloat("m_l_trim", cfg.motor_l_trim);
  prefs.putFloat("m_r_trim", cfg.motor_r_trim);
  prefs.putUChar("min_pwm",  cfg.min_pwm);
  prefs.putFloat("kp_bal",   cfg.kp_bal);
  prefs.putFloat("ki_bal",   cfg.ki_bal);
  prefs.putFloat("kd_bal",   cfg.kd_bal);
  prefs.putUChar("inv_flg",  cfg.invert_flags);
  prefs.end();
  Serial.println("[NVS] Parameters saved to ESP32 Flash memory.");
}

// =============================================================================
// PCNT HARDWARE SETUP (4X Quadrature Decoding)
// =============================================================================
void setupQuadratureUnit(pcnt_unit_t unit, int pinA, int pinB) {
  pinMode(pinA, INPUT);
  pinMode(pinB, INPUT);

  pcnt_config_t cfg_ch0 = {
    .pulse_gpio_num = pinA,
    .ctrl_gpio_num  = pinB,
    .lctrl_mode     = PCNT_MODE_KEEP,
    .hctrl_mode     = PCNT_MODE_REVERSE,
    .pos_mode       = PCNT_COUNT_DEC,
    .neg_mode       = PCNT_COUNT_INC,
    .counter_h_lim  = 32767,
    .counter_l_lim  = -32768,
    .unit           = unit,
    .channel        = PCNT_CHANNEL_0,
  };
  pcnt_unit_config(&cfg_ch0);

  pcnt_config_t cfg_ch1 = {
    .pulse_gpio_num = pinB,
    .ctrl_gpio_num  = pinA,
    .lctrl_mode     = PCNT_MODE_REVERSE,
    .hctrl_mode     = PCNT_MODE_KEEP,
    .pos_mode       = PCNT_COUNT_DEC,
    .neg_mode       = PCNT_COUNT_INC,
    .counter_h_lim  = 32767,
    .counter_l_lim  = -32768,
    .unit           = unit,
    .channel        = PCNT_CHANNEL_1,
  };
  pcnt_unit_config(&cfg_ch1);

  pcnt_set_filter_value(unit, 100);
  pcnt_filter_enable(unit);
  pcnt_counter_pause(unit);
  pcnt_counter_clear(unit);
  pcnt_counter_resume(unit);
}

void updateEncoders() {
  int16_t pL = 0, pR = 0;
  pcnt_get_counter_value(PCNT_UNIT_LEFT, &pL);
  pcnt_get_counter_value(PCNT_UNIT_RIGHT, &pR);

  int16_t diffL = pL - lastPcntL;
  int16_t diffR = pR - lastPcntR;

  // Inversion flags: Bit 2 (Inv Left Enc), Bit 3 (Inv Right Enc)
  if (cfg.invert_flags & 0x04) diffL = -diffL;
  if (cfg.invert_flags & 0x08) diffR = -diffR;

  // Software Swap Encoders: Bit 5 (0x20)
  if (cfg.invert_flags & 0x20) {
    int16_t tmp = diffL;
    diffL = diffR;
    diffR = tmp;
  }

  encLeftAcc += diffL;
  encRightAcc += diffR;
  lastPcntL = pL;
  lastPcntR = pR;
}

void resetEncoders() {
  pcnt_counter_pause(PCNT_UNIT_LEFT);
  pcnt_counter_pause(PCNT_UNIT_RIGHT);
  pcnt_counter_clear(PCNT_UNIT_LEFT);
  pcnt_counter_clear(PCNT_UNIT_RIGHT);
  lastPcntL = 0;
  lastPcntR = 0;
  encLeftAcc = 0;
  encRightAcc = 0;
  pcnt_counter_resume(PCNT_UNIT_LEFT);
  pcnt_counter_resume(PCNT_UNIT_RIGHT);
}

// =============================================================================
// TB6612FNG MOTOR CONTROLLER
// =============================================================================
void setupMotors() {
  pinMode(L_IN1, OUTPUT);
  pinMode(L_IN2, OUTPUT);
  pinMode(R_IN1, OUTPUT);
  pinMode(R_IN2, OUTPUT);

#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  ledcAttach(PWM_L, PWM_FREQ, PWM_RES);
  ledcAttach(PWM_R, PWM_FREQ, PWM_RES);
#else
  ledcSetup(0, PWM_FREQ, PWM_RES);
  ledcAttachPin(PWM_L, 0);
  ledcSetup(1, PWM_FREQ, PWM_RES);
  ledcAttachPin(PWM_R, 1);
#endif

  digitalWrite(L_IN1, LOW);
  digitalWrite(L_IN2, LOW);
  digitalWrite(R_IN1, LOW);
  digitalWrite(R_IN2, LOW);
}

void setMotorsRaw(int speedL, int speedR) {
  // Software Swap Motors: Bit 4 (0x10)
  if (cfg.invert_flags & 0x10) {
    int tmp = speedL;
    speedL = speedR;
    speedR = tmp;
  }

  // Apply trim multipliers
  speedL = (int)(speedL * cfg.motor_l_trim);
  speedR = (int)(speedR * cfg.motor_r_trim);

  // Apply motor direction inversions: Bit 0 (Left), Bit 1 (Right)
  if (cfg.invert_flags & 0x01) speedL = -speedL;
  if (cfg.invert_flags & 0x02) speedR = -speedR;

  // Deadband compensation: ensure motor starts above static gearbox stiction
  if (speedL != 0 && cfg.min_pwm > 0) {
    int sgn = (speedL > 0) ? 1 : -1;
    int mag = map(abs(speedL), 1, 255, cfg.min_pwm, 255);
    speedL = sgn * constrain(mag, (int)cfg.min_pwm, 255);
  }
  if (speedR != 0 && cfg.min_pwm > 0) {
    int sgn = (speedR > 0) ? 1 : -1;
    int mag = map(abs(speedR), 1, 255, cfg.min_pwm, 255);
    speedR = sgn * constrain(mag, (int)cfg.min_pwm, 255);
  }

  speedL = constrain(speedL, -255, 255);
  speedR = constrain(speedR, -255, 255);

  currentPwmL = (int16_t)speedL;
  currentPwmR = (int16_t)speedR;

  // Left Motor H-Bridge
  if (speedL > 0) {
    digitalWrite(L_IN1, HIGH); digitalWrite(L_IN2, LOW);
  } else if (speedL < 0) {
    digitalWrite(L_IN1, LOW);  digitalWrite(L_IN2, HIGH);
  } else {
    digitalWrite(L_IN1, LOW);  digitalWrite(L_IN2, LOW); // Coast
  }

  // Right Motor H-Bridge
  if (speedR > 0) {
    digitalWrite(R_IN1, HIGH); digitalWrite(R_IN2, LOW);
  } else if (speedR < 0) {
    digitalWrite(R_IN1, LOW);  digitalWrite(R_IN2, HIGH);
  } else {
    digitalWrite(R_IN1, LOW);  digitalWrite(R_IN2, LOW); // Coast
  }

#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  ledcWrite(PWM_L, abs(speedL));
  ledcWrite(PWM_R, abs(speedR));
#else
  ledcWrite(0, abs(speedL));
  ledcWrite(1, abs(speedR));
#endif
}

void brakeMotors() {
  // Active short-to-ground electric brake
  digitalWrite(L_IN1, HIGH);
  digitalWrite(L_IN2, HIGH);
  digitalWrite(R_IN1, HIGH);
  digitalWrite(R_IN2, HIGH);
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  ledcWrite(PWM_L, 255);
  ledcWrite(PWM_R, 255);
#else
  ledcWrite(0, 255);
  ledcWrite(1, 255);
#endif
  delay(35);
  // Release to idle
  digitalWrite(L_IN1, LOW);
  digitalWrite(L_IN2, LOW);
  digitalWrite(R_IN1, LOW);
  digitalWrite(R_IN2, LOW);
#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3, 0, 0)
  ledcWrite(PWM_L, 0);
  ledcWrite(PWM_R, 0);
#else
  ledcWrite(0, 0);
  ledcWrite(1, 0);
#endif
  currentPwmL = 0;
  currentPwmR = 0;
  currentSteer = 0.0f;
}

// =============================================================================
// CLOSED-LOOP STRAIGHT BALANCING & MOTION ENGINE
// =============================================================================
void startDistanceMove(bool forward, float distMm, int speedPwm) {
  updateEncoders();
  startEncL = encLeftAcc;
  startEncR = encRightAcc;
  targetDistMm = distMm;
  targetDistTicks = (int32_t)(distMm * cfg.ticks_per_mm);
  motionBasePwm = constrain(speedPwm, 40, 255);
  balIntegral = 0.0f;
  lastBalError = 0.0f;
  lastBalTime = millis();
  motionStartTime = millis();
  distTraveledMm = 0.0f;

  motionState = forward ? STATE_MOVE_FWD : STATE_MOVE_REV;
  Serial.printf("[MOTION] Started %s move: %.1f mm (Target Ticks: %d, Base PWM: %d)\n",
                forward ? "FORWARD" : "BACKWARD", distMm, targetDistTicks, motionBasePwm);
}

void stopAllMotion() {
  brakeMotors();
  motionState = STATE_IDLE;
  Serial.println("[MOTION] Motion STOPPED.");
}

void updateMotionLoop() {
  updateEncoders();

  if (motionState == STATE_IDLE) {
    return;
  }

  // 1. SOLO MOTOR TESTS
  if (motionState == STATE_TEST_SOLO_L) {
    if (millis() - motionStartTime >= motionDurationMs) {
      stopAllMotion();
    } else {
      setMotorsRaw(motionBasePwm, 0);
    }
    return;
  }

  if (motionState == STATE_TEST_SOLO_R) {
    if (millis() - motionStartTime >= motionDurationMs) {
      stopAllMotion();
    } else {
      setMotorsRaw(0, motionBasePwm);
    }
    return;
  }

  // 2. OPEN LOOP FRICTION TEST (No PID)
  if (motionState == STATE_OPEN_LOOP_TEST) {
    if (millis() - motionStartTime >= motionDurationMs) {
      brakeMotors();
      updateEncoders();
      openLoopTicksL = encLeftAcc - startEncL;
      openLoopTicksR = encRightAcc - startEncR;
      openLoopCompleted = true;
      motionState = STATE_IDLE;
      Serial.printf("[DIAG] Open-loop test complete! L Ticks: %d, R Ticks: %d\n", openLoopTicksL, openLoopTicksR);
    } else {
      setMotorsRaw(motionBasePwm, motionBasePwm);
    }
    return;
  }

  // 3. CLOSED-LOOP DISTANCE RUNNER (FORWARD & BACKWARD)
  if (motionState == STATE_MOVE_FWD || motionState == STATE_MOVE_REV) {
    bool isFwd = (motionState == STATE_MOVE_FWD);

    // Calculate traveled ticks for each wheel since move started
    int32_t dL = abs(encLeftAcc - startEncL);
    int32_t dR = abs(encRightAcc - startEncR);
    int32_t avgTicks = (dL + dR) / 2;
    distTraveledMm = (float)avgTicks / cfg.ticks_per_mm;

    // Check if target reached
    if (avgTicks >= targetDistTicks) {
      lastCompletedTraveledTicks = avgTicks;
      brakeMotors();
      motionState = STATE_IDLE;
      Serial.printf("[MOTION] Target reached! Traveled: %.1f mm (%d ticks). L: %d, R: %d\n",
                    distTraveledMm, avgTicks, dL, dR);
      return;
    }

    // --- Smooth Trapezoidal Velocity Profiling ---
    int32_t accelTicks = (int32_t)(25.0f * cfg.ticks_per_mm); // 25mm ramp-up
    int32_t decelTicks = (int32_t)(35.0f * cfg.ticks_per_mm); // 35mm ramp-down
    int32_t remainingTicks = targetDistTicks - avgTicks;

    int currentBasePwm = motionBasePwm;
    if (avgTicks < accelTicks) {
      // Ramp up from min_pwm to motionBasePwm
      currentBasePwm = map(avgTicks, 0, accelTicks, cfg.min_pwm, motionBasePwm);
    } else if (remainingTicks < decelTicks) {
      // Ramp down from motionBasePwm to min_pwm + 10
      int minDecel = max((int)cfg.min_pwm + 10, 55);
      currentBasePwm = map(remainingTicks, 0, decelTicks, minDecel, motionBasePwm);
      currentBasePwm = max(minDecel, currentBasePwm);
    }

    // --- Encoder Straight Balancing PID (Zero Gyro) ---
    // Error = (Right Distance - Left Distance)
    // If Right > Left: Left is lagging (tighter gearbox), error > 0
    float error = (float)(dR - dL);

    unsigned long now = millis();
    float dt = (now - lastBalTime) / 1000.0f;
    if (dt <= 0.0001f) dt = 0.005f;
    lastBalTime = now;

    // Anti-windup integral
    balIntegral += error * dt;
    balIntegral = constrain(balIntegral, -50.0f, 50.0f);

    float dError = (error - lastBalError) / dt;
    lastBalError = error;

    float steer = (error * cfg.kp_bal) + (balIntegral * cfg.ki_bal) + (dError * cfg.kd_bal);
    steer = constrain(steer, -80.0f, 80.0f);
    currentSteer = steer;

    int speedL, speedR;
    if (isFwd) {
      // If Left is tight (error > 0, steer > 0), give Left MORE power, Right LESS power
      speedL = currentBasePwm + (int)steer;
      speedR = currentBasePwm - (int)steer;
    } else {
      // Moving backward: negative PWM
      // If Left is tight (error > 0, steer > 0), Left needs MORE reverse power (more negative)
      speedL = -(currentBasePwm + (int)steer);
      speedR = -(currentBasePwm - (int)steer);
    }

    setMotorsRaw(speedL, speedR);
  }
}

// =============================================================================
// WEB SERVER HANDLERS & REST API
// =============================================================================
void handleRoot() {
  server.send_P(200, "text/html", INDEX_HTML);
}

void handleStatus() {
  updateEncoders();
  const char* stateStr = "IDLE";
  if (motionState == STATE_MOVE_FWD) stateStr = "MOVING FWD";
  else if (motionState == STATE_MOVE_REV) stateStr = "MOVING REV";
  else if (motionState == STATE_TEST_SOLO_L) stateStr = "TESTING L";
  else if (motionState == STATE_TEST_SOLO_R) stateStr = "TESTING R";
  else if (motionState == STATE_OPEN_LOOP_TEST) stateStr = "OPEN LOOP TEST";

  char json[380];
  snprintf(json, sizeof(json),
    "{\"state\":\"%s\",\"enc_l\":%ld,\"enc_r\":%ld,\"pwm_l\":%d,\"pwm_r\":%d,\"steer\":%.1f,"
    "\"target_dist\":%.1f,\"dist_traveled\":%.1f,\"ticks_mm\":%.2f,"
    "\"trim_l\":%.2f,\"trim_r\":%.2f,\"min_pwm\":%d,"
    "\"kp_bal\":%.2f,\"ki_bal\":%.3f,\"kd_bal\":%.2f,\"inv_flags\":%d}",
    stateStr, (long)encLeftAcc, (long)encRightAcc, currentPwmL, currentPwmR, currentSteer,
    targetDistMm, distTraveledMm, cfg.ticks_per_mm,
    cfg.motor_l_trim, cfg.motor_r_trim, cfg.min_pwm,
    cfg.kp_bal, cfg.ki_bal, cfg.kd_bal, cfg.invert_flags
  );
  server.send(200, "application/json", json);
}

void handleMove() {
  String dir = server.hasArg("dir") ? server.arg("dir") : "fwd";
  float dist = server.hasArg("dist") ? server.arg("dist").toFloat() : 180.0f;
  int speed  = server.hasArg("speed") ? server.arg("speed").toInt() : 120;
  
  if (dist <= 0) dist = 180.0f;
  speed = constrain(speed, 40, 255);

  startDistanceMove((dir == "fwd"), dist, speed);
  server.send(200, "application/json", "{\"status\":\"started\"}");
}

void handleStop() {
  stopAllMotion();
  server.send(200, "application/json", "{\"status\":\"stopped\"}");
}

void handleResetEnc() {
  resetEncoders();
  server.send(200, "application/json", "{\"status\":\"reset\"}");
}

void handleTestSolo() {
  String motor = server.hasArg("motor") ? server.arg("motor") : "L";
  int speed    = server.hasArg("speed") ? server.arg("speed").toInt() : 120;
  int dur      = server.hasArg("dur") ? server.arg("dur").toInt() : 1000;

  motionBasePwm = constrain(speed, 40, 255);
  motionDurationMs = constrain(dur, 200, 5000);
  motionStartTime = millis();
  motionState = (motor == "L") ? STATE_TEST_SOLO_L : STATE_TEST_SOLO_R;

  server.send(200, "application/json", "{\"status\":\"solo_started\"}");
}

void handleOpenLoop() {
  int speed = server.hasArg("speed") ? server.arg("speed").toInt() : 100;
  int dur   = server.hasArg("dur") ? server.arg("dur").toInt() : 2000;

  updateEncoders();
  startEncL = encLeftAcc;
  startEncR = encRightAcc;
  openLoopCompleted = false;
  motionBasePwm = constrain(speed, 40, 255);
  motionDurationMs = constrain(dur, 500, 5000);
  motionStartTime = millis();
  motionState = STATE_OPEN_LOOP_TEST;

  server.send(200, "application/json", "{\"status\":\"open_loop_started\"}");
}

void handleOpenLoopResult() {
  char json[120];
  snprintf(json, sizeof(json),
    "{\"completed\":%s,\"ticks_l\":%ld,\"ticks_r\":%ld}",
    openLoopCompleted ? "true" : "false", (long)openLoopTicksL, (long)openLoopTicksR
  );
  server.send(200, "application/json", json);
}

void handleSetConfig() {
  if (server.hasArg("trim_l"))   cfg.motor_l_trim = server.arg("trim_l").toFloat();
  if (server.hasArg("trim_r"))   cfg.motor_r_trim = server.arg("trim_r").toFloat();
  if (server.hasArg("min_pwm"))  cfg.min_pwm      = (uint8_t)server.arg("min_pwm").toInt();
  if (server.hasArg("ticks_mm")) cfg.ticks_per_mm = server.arg("ticks_mm").toFloat();
  if (server.hasArg("kp_bal"))   cfg.kp_bal       = server.arg("kp_bal").toFloat();
  if (server.hasArg("ki_bal"))   cfg.ki_bal       = server.arg("ki_bal").toFloat();
  if (server.hasArg("kd_bal"))   cfg.kd_bal       = server.arg("kd_bal").toFloat();

  if (server.hasArg("save") && server.arg("save") == "1") {
    saveConfig();
  }
  server.send(200, "application/json", "{\"status\":\"updated\"}");
}

void handleSetInverts() {
  if (server.hasArg("flags")) {
    cfg.invert_flags = (uint8_t)server.arg("flags").toInt();
    saveConfig();
  }
  server.send(200, "application/json", "{\"status\":\"inverts_saved\"}");
}

void handleCalibrateRuler() {
  float actualMm = server.hasArg("actual_mm") ? server.arg("actual_mm").toFloat() : 0.0f;
  if (actualMm > 5.0f && lastCompletedTraveledTicks > 50) {
    cfg.ticks_per_mm = (float)lastCompletedTraveledTicks / actualMm;
    saveConfig();
    char json[100];
    snprintf(json, sizeof(json), "{\"status\":\"calibrated\",\"ticks_per_mm\":%.3f}", cfg.ticks_per_mm);
    server.send(200, "application/json", json);
  } else {
    server.send(400, "application/json", "{\"error\":\"invalid_distance_or_no_prior_move\"}");
  }
}

// =============================================================================
// OLED DISPLAY DIAGNOSTICS (Optional SSD1306)
// =============================================================================
void initOled() {
  Wire.begin(21, 22);
  Wire.setClock(400000);

  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C) || display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
    oledFound = true;
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(14, 10);
    display.println("MICROMOUSE TUNER");
    display.drawFastHLine(0, 22, 128, SSD1306_WHITE);
    display.setCursor(8, 30);
    display.printf("AP: %s\n", AP_SSID);
    display.setCursor(8, 44);
    display.printf("IP: %s\n", WiFi.softAPIP().toString().c_str());
    display.display();
  }
}

void updateOledTelemetry() {
  if (!oledFound) return;
  static unsigned long lastOled = 0;
  if (millis() - lastOled < 200) return;
  lastOled = millis();

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.printf("IP: %s\n", WiFi.softAPIP().toString().c_str());
  display.drawFastHLine(0, 10, 128, SSD1306_WHITE);

  display.setCursor(0, 15);
  display.printf("L: %-6ld R: %-6ld\n", (long)encLeftAcc, (long)encRightAcc);
  display.setCursor(0, 28);
  display.printf("Diff: %+ld ticks\n", (long)(encLeftAcc - encRightAcc));
  display.setCursor(0, 40);
  display.printf("PWM: L:%-3d R:%-3d\n", currentPwmL, currentPwmR);
  display.setCursor(0, 52);
  const char* st = (motionState == STATE_IDLE) ? "IDLE" :
                   (motionState == STATE_MOVE_FWD) ? "RUN FWD" :
                   (motionState == STATE_MOVE_REV) ? "RUN REV" : "TESTING";
  display.printf("Dist:%.0fmm [%s]\n", distTraveledMm, st);
  display.display();
}

// =============================================================================
// ARDUINO SETUP & LOOP
// =============================================================================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n\n==================================================");
  Serial.println("   MICROMOUSE MOTOR & ENCODER TUNER (STANDALONE)");
  Serial.println("==================================================");

  // 1. Load calibration parameters from Flash
  loadConfig();

  // 2. Initialize Quadrature Encoders (PCNT)
  setupQuadratureUnit(PCNT_UNIT_LEFT,  ENC_L_PULSE_PIN, ENC_L_CTRL_PIN);
  setupQuadratureUnit(PCNT_UNIT_RIGHT, ENC_R_PULSE_PIN, ENC_R_CTRL_PIN);
  resetEncoders();

  // 3. Initialize Motors (TB6612FNG)
  setupMotors();

  // 4. Initialize Wi-Fi SoftAP
  WiFi.mode(WIFI_AP_STA);
  bool apOk = WiFi.softAP(AP_SSID, (strlen(AP_PASS) >= 8) ? AP_PASS : NULL);
  if (apOk) {
    Serial.printf("[WIFI] Standalone Hotspot Active!\n");
    Serial.printf("[WIFI] SSID : %s\n", AP_SSID);
    Serial.printf("[WIFI] PASS : %s\n", strlen(AP_PASS) >= 8 ? AP_PASS : "(Open Network)");
    Serial.printf("[WIFI] IP   : %s\n", WiFi.softAPIP().toString().c_str());
  }

  // Connect to STA Wi-Fi if configured
  if (strlen(STA_SSID) > 0) {
    WiFi.begin(STA_SSID, STA_PASS);
    Serial.printf("[WIFI] Connecting to %s...\n", STA_SSID);
  }

  // 5. Initialize Web Server Routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/move", HTTP_GET, handleMove);
  server.on("/stop", HTTP_GET, handleStop);
  server.on("/reset_enc", HTTP_GET, handleResetEnc);
  server.on("/test_solo", HTTP_GET, handleTestSolo);
  server.on("/open_loop", HTTP_GET, handleOpenLoop);
  server.on("/open_loop_result", HTTP_GET, handleOpenLoopResult);
  server.on("/set_config", HTTP_GET, handleSetConfig);
  server.on("/set_inverts", HTTP_GET, handleSetInverts);
  server.on("/calibrate_ruler", HTTP_GET, handleCalibrateRuler);
  server.begin();
  Serial.println("[WEB] Web Server started on port 80.");

  // 6. Optional OLED
  initOled();

  Serial.println("==================================================");
  Serial.println("Ready! Connect phone/PC to 'MicroMouse-Tuner' Wi-Fi");
  Serial.println("and navigate to http://192.168.4.1 in your browser.");
  Serial.println("==================================================\n");
}

void loop() {
  // 1. Handle incoming HTTP requests from the browser
  server.handleClient();

  // 2. High-speed non-blocking motion controller (~200 Hz)
  static unsigned long lastMotionTick = 0;
  unsigned long now = millis();
  if (now - lastMotionTick >= 5) {
    lastMotionTick = now;
    updateMotionLoop();
  }

  // 3. Update OLED display telemetry (if attached)
  updateOledTelemetry();
}
