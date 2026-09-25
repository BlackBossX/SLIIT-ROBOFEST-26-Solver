#pragma once
#include <Arduino.h>

// =========================================================
//  ADC Pin Assignments for IR sensors and voltage monitor.
//
//  IMPORTANT: On ESP32, GPIO 34/35/36/39 are INPUT-ONLY
//  and ideal for ADC use (no internal pull-ups/pull-downs).
//
//  GPIO 34 = ADC1_CH6
//  GPIO 35 = ADC1_CH7
//  GPIO 36 = ADC1_CH0 (VP)
//  GPIO 39 = ADC1_CH3 (VN)
//
//  Adjust these pins to match your actual wiring.
// =========================================================
#define PIN_LF_SENSOR   34   // Left  Front IR sensor (ADC)
#define PIN_RF_SENSOR   35   // Right Front IR sensor (ADC)
#define PIN_DL_SENSOR   36   // Diagonal Left  IR sensor (ADC)
#define PIN_DR_SENSOR   39   // Diagonal Right IR sensor (ADC)
#define PIN_GYRO_Z      15   // Gyroscope Z-axis analog output (ADC2_CH3)
#define PIN_VOL_METER   12   // Battery voltage divider (ADC2_CH5)

// =========================================================
//  ADC read macros — replace the original STM32 readADC macros
// =========================================================
#define read_LF_Sensor   analogRead(PIN_LF_SENSOR)
#define read_RF_Sensor   analogRead(PIN_RF_SENSOR)
#define read_DL_Sensor   analogRead(PIN_DL_SENSOR)
#define read_DR_Sensor   analogRead(PIN_DR_SENSOR)
#define read_Outz        analogRead(PIN_GYRO_Z)
#define read_Vol_Meter   analogRead(PIN_VOL_METER)

// =========================================================
//  Sensor value globals — extern declarations
// =========================================================
extern int     reflectionRate;
extern int32_t volMeter;
extern int32_t voltage;
extern int32_t LFSensor;
extern int32_t RFSensor;
extern int32_t DLSensor;
extern int32_t DRSensor;
extern int32_t aSpeed;
extern int32_t angle;

// =========================================================
//  Function prototypes
// =========================================================
void readSensor(void);
void readGyro(void);
void readVolMeter(void);
void lowBatCheck(void);
void sensor_Configuration(void);
