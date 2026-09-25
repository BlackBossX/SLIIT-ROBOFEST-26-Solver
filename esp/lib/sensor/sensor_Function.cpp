#include "sensor_Function.h"

// =========================================================
//  Six VL53L0X sensor objects
// =========================================================
static VL53L0X sensors[NUM_SENSORS];

static bool sensorActive[NUM_SENSORS] = {false, false, false, false, false, false};

// =========================================================
//  Global readings (mm). 9999 = invalid / no echo.
// =========================================================
uint16_t sensorMM[NUM_SENSORS] = {SENSOR_INVALID, SENSOR_INVALID,
                                   SENSOR_INVALID, SENSOR_INVALID,
                                   SENSOR_INVALID, SENSOR_INVALID};

// =========================================================
//  Sensor_Configuration
//
//  Boot sequence:
//  1. Pull all XSHUT pins LOW  → all sensors in shutdown.
//  2. For each sensor i:
//     a. Drive XSHUT[i] HIGH   → sensor i wakes up at 0x29.
//     b. Wait for sensor to boot.
//     c. Change its I²C address to SENSOR_ADDRS[i].
//     d. Start continuous ranging.
//  3. Now all 6 sensors are alive on unique addresses.
//
//  Returns false if any sensor fails to initialize.
// =========================================================
bool Sensor_Configuration(void) {
    Wire.begin(SENSOR_SDA_PIN, SENSOR_SCL_PIN);

    // Put all sensors into shutdown by pulling XSHUT LOW
    for (uint8_t i = 0; i < NUM_SENSORS; i++) {
        pinMode(XSHUT_PINS[i], OUTPUT);
        digitalWrite(XSHUT_PINS[i], LOW);
    }
    delay(10);  // Let all sensors fully reset

    bool ok = true;
    for (uint8_t i = 0; i < NUM_SENSORS; i++) {
        // Wake this sensor up
        pinMode(XSHUT_PINS[i], OUTPUT);
        digitalWrite(XSHUT_PINS[i], HIGH);
        delay(10);  // Boot time

        sensors[i].setTimeout(500); // Increased timeout for init
        if (!sensors[i].init()) {
            Serial.printf("[SENSOR] ERROR: Sensor %d (XSHUT GPIO%d) failed to init!\n",
                          i, XSHUT_PINS[i]);
            ok = false;
            continue;
        }
        sensorActive[i] = true;
        // Assign unique address
        sensors[i].setAddress(SENSOR_ADDRS[i]);

        // Use high-speed continuous mode (reduces per-reading overhead)
        sensors[i].setMeasurementTimingBudget(20000); // 20ms budget
        sensors[i].startContinuous();
        
        sensors[i].setTimeout(SENSOR_TIMEOUT_MS); // Restore short timeout for continuous reads

        Serial.printf("[SENSOR] Sensor %d OK  XSHUT=GPIO%d  addr=0x%02X\n",
                      i, XSHUT_PINS[i], SENSOR_ADDRS[i]);
    }
    return ok;
}

// =========================================================
//  readSensors — update all 6 global readings (non-blocking
//  read from continuous mode — returns last completed range)
// =========================================================
void readSensors(void) {
    for (uint8_t i = 0; i < NUM_SENSORS; i++) {
        if (!sensorActive[i]) {
            sensorMM[i] = SENSOR_INVALID;
            continue;
        }
        uint16_t raw = sensors[i].readRangeContinuousMillimeters();
        if (sensors[i].timeoutOccurred() || raw > SENSOR_MAX_MM) {
            sensorMM[i] = SENSOR_INVALID;
        } else {
            sensorMM[i] = raw;
        }
    }
}

// =========================================================
//  getSensor — safe single-sensor access with bounds check
// =========================================================
uint16_t getSensor(uint8_t idx) {
    if (idx >= NUM_SENSORS) return SENSOR_INVALID;
    return sensorMM[idx];
}

// =========================================================
//  Wall detection helpers (used by maze solver)
// =========================================================
bool isWallFront(uint16_t threshold_mm) {
    // Wall in front if BOTH front sensors see something
    return (sensorMM[SENSOR_0R] < threshold_mm) ||
           (sensorMM[SENSOR_0L] < threshold_mm);
}

bool isWallRight(uint16_t threshold_mm) {
    return sensorMM[SENSOR_90R] < threshold_mm;
}

bool isWallLeft(uint16_t threshold_mm) {
    return sensorMM[SENSOR_90L] < threshold_mm;
}
