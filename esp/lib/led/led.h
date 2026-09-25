#pragma once
#include <Arduino.h>

// =========================================================
//  LED GPIO Pin Assignments  (adjust to match your wiring)
// =========================================================
#define PIN_LED1   5
#define PIN_LED2   16
#define PIN_LED3   17
#define PIN_LED4   21
#define PIN_LED5   13

// =========================================================
//  IR Emitter GPIO Pin Assignments
// =========================================================
#define PIN_LF_EM   32   // Left  Front IR emitter
#define PIN_RF_EM   33   // Right Front IR emitter
#define PIN_SIDE_EM  4   // Diagonal (side) IR emitters

// =========================================================
//  LED macros  (match the style of the original STM32 code)
// =========================================================
#define LED1_ON    digitalWrite(PIN_LED1, HIGH)
#define LED1_OFF   digitalWrite(PIN_LED1, LOW)
#define LED2_ON    digitalWrite(PIN_LED2, HIGH)
#define LED2_OFF   digitalWrite(PIN_LED2, LOW)
#define LED3_ON    digitalWrite(PIN_LED3, HIGH)
#define LED3_OFF   digitalWrite(PIN_LED3, LOW)
#define LED4_ON    digitalWrite(PIN_LED4, HIGH)
#define LED4_OFF   digitalWrite(PIN_LED4, LOW)
#define LED5_ON    digitalWrite(PIN_LED5, HIGH)
#define LED5_OFF   digitalWrite(PIN_LED5, LOW)

#define ALL_LED_ON  { LED1_ON;  LED2_ON;  LED3_ON;  LED4_ON;  LED5_ON;  }
#define ALL_LED_OFF { LED1_OFF; LED2_OFF; LED3_OFF; LED4_OFF; LED5_OFF; }

// =========================================================
//  IR Emitter macros
// =========================================================
#define LF_EM_ON    digitalWrite(PIN_LF_EM,   HIGH)
#define LF_EM_OFF   digitalWrite(PIN_LF_EM,   LOW)
#define RF_EM_ON    digitalWrite(PIN_RF_EM,   HIGH)
#define RF_EM_OFF   digitalWrite(PIN_RF_EM,   LOW)
#define SIDE_EM_ON  digitalWrite(PIN_SIDE_EM, HIGH)
#define SIDE_EM_OFF digitalWrite(PIN_SIDE_EM, LOW)

// =========================================================
//  Function prototypes
// =========================================================
void LED_Configuration(void);
