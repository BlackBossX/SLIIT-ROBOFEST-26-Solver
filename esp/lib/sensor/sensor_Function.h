#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <VL53L0X.h>

// =========================================================
//  VL53L0X ToF Sensor Configuration
//
//  6 sensors in order: Right-to-Left
//    [0] 90R  — 90° Right  (side-right)
//    [1] 45R  — 45° Right  (diagonal-right)
//    [2]  0R  — 0° Right   (front-right)
//    [3]  0L  — 0° Left    (front-left)
//    [4] 45L  — 45° Left   (diagonal-left)
//    [5] 90L  — 90° Left   (side-left)
//
//  XSHUT pins (used to boot sensors one at a time and assign
//  unique I²C addresses):
// =========================================================
#define NUM_SENSORS      6

// XSHUT pin for each sensor (index matches above order)
// Right to Left: 90R, 45R, 0R, 0L, 45L, 90L
const uint8_t XSHUT_PINS[NUM_SENSORS] = {13, 14, 25, 26, 27, 32};

// Unique I²C addresses assigned at runtime
const uint8_t SENSOR_ADDRS[NUM_SENSORS] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35};

// I²C bus pins
#define SENSOR_SDA_PIN   21
#define SENSOR_SCL_PIN   22

// =========================================================
//  Tunable: sensor reading timeout / invalid value
// =========================================================
#define SENSOR_TIMEOUT_MS    30    // Per-sensor ranging timeout (ms)
#define SENSOR_MAX_MM       1200   // Readings above this are treated as "open" (mm)
#define SENSOR_INVALID       9999  // Value reported when sensor returns error/timeout

// =========================================================
//  Named indices for easy access
// =========================================================
#define SENSOR_90R   0
#define SENSOR_45R   1
#define SENSOR_0R    2   // Front-right
#define SENSOR_0L    3   // Front-left
#define SENSOR_45L   4
#define SENSOR_90L   5

// =========================================================
//  Global sensor readings (mm). Updated by readSensors().
// =========================================================
extern uint16_t sensorMM[NUM_SENSORS];

// =========================================================
//  Function prototypes
// =========================================================
bool    Sensor_Configuration(void);         // Call once in setup()
void    readSensors(void);                  // Read all 6 sensors
uint16_t getSensor(uint8_t idx);            // Get reading by index
bool    isWallFront(uint16_t threshold_mm); // True if wall detected ahead
bool    isWallRight(uint16_t threshold_mm); // True if wall detected on right
bool    isWallLeft(uint16_t threshold_mm);  // True if wall detected on left
