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
#include "motion.h"
#include "display.h"

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

    // OLED Display
    if (!Display_Init()) {
        Serial.println("[BOOT] WARNING: OLED display failed to initialize!");
    } else {
        Display_Message("Booting up...");
    }

    // ESP-NOW comms (Ground Station receiver)
    Comms_Init();

    Serial.println("[BOOT] Init complete. Robot ready. Waiting 5 seconds before start...");
    printParams();    // Show starting parameter values

    // ---- 5s Start Timer with LED ----
    for (int i = 0; i < 5; i++) {
        char buf[32];
        sprintf(buf, "Starting in %d...", 5 - i);
        Display_Message(buf);

        digitalWrite(PIN_STATUS_LED, HIGH);
        delay(500);
        digitalWrite(PIN_STATUS_LED, LOW);
        delay(500);
        Serial.printf("[BOOT] %d...\r\n", 5 - i);
    }
    Display_Message("Go!");
    Serial.println("[BOOT] Go!");
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

    // ---- Update OLED ----
    // We pass 90L (Left wall), 0L/0R avg (Front wall), 90R (Right wall) to the display
    uint16_t frontAvg = (sensorMM[SENSOR_0L] == 9999 || sensorMM[SENSOR_0R] == 9999) ? 9999 : (sensorMM[SENSOR_0L] + sensorMM[SENSOR_0R])/2;
    Display_Telemetry(sensorMM[SENSOR_90L], frontAvg, sensorMM[SENSOR_90R], gyroZ, leftTicks, rightTicks);

    // ---- Wall detection example ----
    bool wFront = isWallFront(p.wall_front_thresh);
    bool wRight = isWallRight(p.wall_side_thresh);
    bool wLeft  = isWallLeft (p.wall_side_thresh);
    Serial.printf("[WALLS] Front=%d Right=%d Left=%d\r\n",
                  wFront, wRight, wLeft);

    // ---- TEST MODE TOGGLE ----
    // Set this to true if you just want to see sensor readings on the screen
    // without the robot trying to move and block the loop.
    bool test_sensors_only = false; 

    if (!test_sensors_only) {
        // ---- MAZE SOLVING: Left-Hand Rule ----
        // 1. If there is no wall on the left, turn left and step forward
        if (!wLeft) {
            turnLeft90();
            moveForwardOneCell();
        } 
        // 2. Else if there is no wall in front, step forward
        else if (!wFront) {
            moveForwardOneCell();
        } 
        // 3. Else if there is no wall on the right, turn right and step forward
        else if (!wRight) {
            turnRight90();
            moveForwardOneCell();
        } 
        // 4. Dead end: Turn around and step forward
        else {
            turnAround180();
            moveForwardOneCell();
        }
    }
    
    // Brief pause between readings
    delay(100);
}
