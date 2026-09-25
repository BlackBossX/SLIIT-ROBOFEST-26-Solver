#pragma once
#include <Arduino.h>
#include "driver/pcnt.h"  // ESP-IDF PCNT (Pulse Counter) driver

// =========================================================
//  Hardware Quadrature Encoder Configuration
//
//  Uses ESP32 PCNT (Pulse Counter) peripheral directly —
//  no external library needed. PCNT supports full quadrature
//  mode (counts ×4 per electrical cycle).
//
//  Pin assignments (INPUT-ONLY GPIO = ADC-safe, no boot issues):
//    Left  encoder: PULSE = GPIO34,  CTRL = GPIO36
//    Right encoder: PULSE = GPIO39,  CTRL = GPIO35
// =========================================================

// Left encoder
#define ENC_L_PULSE_PIN   34   // Phase A (input-only GPIO)
#define ENC_L_CTRL_PIN    36   // Phase B (input-only GPIO)
#define PCNT_UNIT_LEFT    PCNT_UNIT_0

// Right encoder
#define ENC_R_PULSE_PIN   39   // Phase A (input-only GPIO)
#define ENC_R_CTRL_PIN    35   // Phase B (input-only GPIO)
#define PCNT_UNIT_RIGHT   PCNT_UNIT_1

// PCNT counter limits (16-bit hardware counter, saturates at ±32767)
// We read and accumulate into a 32-bit software counter before saturation.
#define PCNT_HIGH_LIMIT   32000
#define PCNT_LOW_LIMIT   -32000

// =========================================================
//  Function prototypes
// =========================================================
void     Encoder_Configuration(void);

int32_t  getLeftEncCount(void);     // Cumulative left  encoder ticks
int32_t  getRightEncCount(void);    // Cumulative right encoder ticks

void     resetLeftEncCount(void);   // Zero left  count
void     resetRightEncCount(void);  // Zero right count

void     updateEncoders(void);      // Call frequently to accumulate overflow
