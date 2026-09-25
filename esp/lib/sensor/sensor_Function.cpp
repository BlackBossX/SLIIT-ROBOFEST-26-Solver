#include "sensor_Function.h"
#include "led.h"
#include "pwm.h"

// =========================================================
//  Global sensor state variables
//  Mirrors the original globals in sensor_Function.c
// =========================================================
int     reflectionRate = 1000; // 1.000 scale factor (kept as integer * 1000)
int32_t volMeter = 0;
int32_t voltage  = 0;
int32_t LFSensor = 0;
int32_t RFSensor = 0;
int32_t DLSensor = 0;
int32_t DRSensor = 0;
int32_t aSpeed   = 0;  // Angular velocity (degrees/s * some scale)
int32_t angle    = 0;  // Accumulated heading angle

// =========================================================
//  sensor_Configuration — configure ADC resolution
//  ESP32 default is 12-bit (0-4095), matching STM32 12-bit ADC.
// =========================================================
void sensor_Configuration(void) {
    analogReadResolution(12);   // 12-bit: 0 - 4095
    analogSetAttenuation(ADC_11db); // Full range ~0-3.3V
}

// =========================================================
//  readSensor — reads all 4 IR sensors with ambient subtraction
//
//  PORTED LOGIC (identical to original):
//  1. Read ADC with emitter OFF (ambient baseline)
//  2. Turn emitter ON, wait 60µs, read again
//  3. Subtract baseline to get reflection value only
//  4. Apply reflectionRate calibration factor
//
//  Timing: original uses elapseMicros() based on a shared
//  start time. On ESP32 we replicate with delayMicroseconds().
//  Total cycle: ~400µs (fits in a 1ms ISR with headroom)
// =========================================================
void readSensor(void) {
    // --- Step 1: Read ambient baseline (emitters OFF) ---
    LFSensor = read_LF_Sensor;
    RFSensor = read_RF_Sensor;
    DLSensor = read_DL_Sensor;
    DRSensor = read_DR_Sensor;

    uint32_t startUs = micros();

    // --- Left Front sensor ---
    LF_EM_ON;
    // Wait 60µs from start before reading
    while ((micros() - startUs) < 60);
    LFSensor = read_LF_Sensor - LFSensor;
    LF_EM_OFF;
    if (LFSensor < 0) LFSensor = 0;  // Error check
    while ((micros() - startUs) < 140);  // Wait until 140µs mark

    // --- Right Front sensor ---
    RF_EM_ON;
    while ((micros() - startUs) < 200);
    RFSensor = read_RF_Sensor - RFSensor;
    RF_EM_OFF;
    if (RFSensor < 0) RFSensor = 0;
    while ((micros() - startUs) < 280);  // Wait until 280µs mark

    // --- Diagonal (side) sensors ---
    SIDE_EM_ON;
    while ((micros() - startUs) < 340);
    DLSensor = read_DL_Sensor - DLSensor;
    DRSensor = read_DR_Sensor - DRSensor;
    SIDE_EM_OFF;
    if (DLSensor < 0) DLSensor = 0;
    if (DRSensor < 0) DRSensor = 0;

    // --- Apply calibration / reflection rate ---
    LFSensor = LFSensor * reflectionRate / 1000;
    RFSensor = RFSensor * reflectionRate / 1000;
    DLSensor = DLSensor * reflectionRate / 1000;
    DRSensor = DRSensor * reflectionRate / 1000;
}
// There are roughly 1000 - 340 = 660µs remaining in a 1ms window after readSensor()

// =========================================================
//  readGyro — reads the analog gyro Z-axis and integrates angle
//
//  PORTED LOGIC (identical to original):
//  Takes 20 samples, averages, removes calibration offset,
//  scales to angular velocity, then integrates into 'angle'.
//
//  NOTE: The calibration constant 92980000 was measured for
//  the original hardware's gyro. You MUST re-calibrate this
//  for your gyro chip and supply voltage on the ESP32.
//  Run with motor off and log aSpeed; it should read ~0
//  when stationary. Adjust the constant accordingly.
// =========================================================
void readGyro(void) {
    int sampleNum = 20;
    aSpeed = 0;
    for (int i = 0; i < sampleNum; i++) {
        aSpeed += read_Outz;
    }
    // Scale up, remove zero-rate offset, scale back down
    aSpeed *= 50000 / sampleNum;
    aSpeed -= 92980000;   // <-- CALIBRATE THIS for your gyro
    aSpeed /= 50000;
    aSpeed /= 4;
    angle += aSpeed;
}

// =========================================================
//  readVolMeter — reads raw battery voltage via ADC
//
//  The original formula: voltage = volMeter * 809 / 3248
//  This gives voltage in mV * 10  (e.g. 8200 = 8.2V)
//
//  NOTE: The voltage divider ratio on your ESP32 PCB may
//  differ from the original STM32 board. Adjust the 809/3248
//  constants to match your actual voltage divider resistors.
//  Make sure input voltage to this pin never exceeds 3.3V!
// =========================================================
void readVolMeter(void) {
    volMeter = read_Vol_Meter;                  // raw 12-bit ADC value (0-4095)
    voltage  = (int32_t)volMeter * 809 / 3248; // convert to Vx1000 (e.g. 7850 = 7.85V)
}

// =========================================================
//  lowBatCheck — stops motors and flashes LED if V < 7.0V
//  Identical logic to original; just uses Arduino APIs.
// =========================================================
void lowBatCheck(void) {
    if (voltage < 7000) {  // Below 7.0V (voltage is in mV units here)
        setLeftPwm(0);
        setRightPwm(0);

        while (1) {
            Serial.println("LOW BATTERY!");
            ALL_LED_OFF;
            delay(200);
            ALL_LED_ON;
            delay(200);
        }
    } else {
        Serial.printf("Battery OK: %d mV\r\n", voltage);
        delay(1000);
    }
}
