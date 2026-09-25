#pragma once
#include <Arduino.h>

// =========================================================
//  Encoder Pin Assignments   (adjust to match your wiring)
//  Both channels must be interrupt-capable GPIOs on ESP32.
// =========================================================
#define PIN_ENC_R_A   18   // Right encoder channel A
#define PIN_ENC_R_B   19   // Right encoder channel B
#define PIN_ENC_L_A   22   // Left  encoder channel A
#define PIN_ENC_L_B   23   // Left  encoder channel B

// =========================================================
//  Function prototypes
// =========================================================
void     Encoder_Configuration(void);
int32_t  getLeftEncCount(void);
int32_t  getRightEncCount(void);
void     resetLeftEncCount(void);
void     resetRightEncCount(void);
