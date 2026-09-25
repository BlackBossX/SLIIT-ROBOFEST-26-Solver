/*
 * ================================================================
 *  Micromouse — Main Robot Controller
 *  Target  : ESP32 (PlatformIO / Arduino framework)
 *  Hardware:
 *    • 6× VL53L0X ToF sensors  (I²C, XSHUT pins 13/14/25/26/27/32)
 *    • TB6612FNG dual H-bridge  (PWMA=23, AIN1=18, AIN2=19,
 *                                PWMB=17, BIN1=4,  BIN2=16)
 *    • Quadrature encoders      (Left:  PULSE=34, CTRL=36)
 *                               (Right: PULSE=39, CTRL=35)
 *    • ESP-NOW receiver         (Ground Station → RobotParams)
 * ================================================================
 *
 *  Build : pio run
 *  Upload: pio run --target upload
 *  Monitor: pio device monitor
 * ================================================================
 */

#include <Arduino.h>
#include "sensor_Function.h"
#include "pwm.h"
#include "encoder.h"
#include "comms.h"
#include "gyro.h"

// Built-in LED (GPIO2) — used for status indication
#define PIN_STATUS_LED   2

// ================================================================
//  setup() — runs once at boot
// ================================================================
void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n=== Micromouse Boot ===");

    // Status LED
    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, HIGH);   // On while initializing

    // Motor driver
    Motor_Init();

    // Quadrature encoders (raw PCNT)
    Encoder_Configuration();

    // VL53L0X ToF sensors
    if (!Sensor_Configuration()) {
        Serial.println("[BOOT] WARNING: One or more sensors failed — check wiring!");
    }

    // MPU6050 Gyro (Must be initialized AFTER Sensor_Configuration because it relies on Wire.begin())
    if (!Gyro_Init()) {
        Serial.println("[BOOT] WARNING: Gyro failed to initialize!");
    }

    // ESP-NOW comms (Ground Station receiver)
    Comms_Init();

    // Print MAC address so Ground Station can target this robot
    Serial.println("[BOOT] Init complete. Robot ready.");
    printParams();    // Show starting parameter values

    digitalWrite(PIN_STATUS_LED, LOW);
}

// ================================================================
//  loop() — main control loop
// ================================================================
void loop() {
    // ---- Pull latest tunable parameters ----
    RobotParams& p = getParams();

    // ---- Check for Ground Station update ----
    if (paramsUpdated()) {
        Serial.println("[MAIN] Parameters updated:");
        printParams();
        // Re-apply any values that affect hardware directly
        // (e.g., encoder direction, speed limits, etc.)
    }

    // ---- Read all 6 ToF sensors ----
    readSensors();

    // ---- Encoder counts ----
    int32_t leftTicks  = getLeftEncCount()  * p.enc_L_direction;
    int32_t rightTicks = getRightEncCount() * p.enc_R_direction;

    // ---- Gyro reading ----
    float gyroZ = getGyroZ();

    // ---- Print telemetry ----
    Serial.printf(
        "[TEL] 90R=%4d 45R=%4d 0R=%4d 0L=%4d 45L=%4d 90L=%4d  "
        "encL=%6ld encR=%6ld  GyroZ=%6.1f\r\n",
        sensorMM[SENSOR_90R],
        sensorMM[SENSOR_45R],
        sensorMM[SENSOR_0R],
        sensorMM[SENSOR_0L],
        sensorMM[SENSOR_45L],
        sensorMM[SENSOR_90L],
        (long)leftTicks,
        (long)rightTicks,
        gyroZ
    );

    // ---- Wall detection example ----
    bool wFront = isWallFront(p.wall_front_thresh);
    bool wRight = isWallRight(p.wall_side_thresh);
    bool wLeft  = isWallLeft (p.wall_side_thresh);
    Serial.printf("[WALLS] Front=%d Right=%d Left=%d\r\n",
                  wFront, wRight, wLeft);

    // ---- TEST DRIVE: drive forward slowly then stop ----
    // Remove / replace this block with your maze solver logic
    setLeftPwm(p.speed_fwd);
    setRightPwm(p.speed_fwd);
    delay(500);
    setLeftPwm(0);
    setRightPwm(0);
    delay(500);
}
