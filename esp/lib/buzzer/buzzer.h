#pragma once
#include <Arduino.h>

// =========================================================
//  Buzzer Pin Assignment    (adjust to match your wiring)
// =========================================================
#define PIN_BUZZER   2   // GPIO2 is the built-in LED on most DevKits;
                          // change if you have a dedicated buzzer pin

// =========================================================
//  LEDC channel for buzzer (use a channel not used by motors)
// =========================================================
#define LEDC_CH_BUZZER   4

// =========================================================
//  Buzzer state variable
//  Decrement this in your 1ms loop/timer to auto-shutoff.
// =========================================================
extern int32_t buzzerTime;

// =========================================================
//  Helper macros — mirror original STM32 buzzer.h style
//  On ESP32 we use LEDC duty to turn sound on/off.
// =========================================================

// 12.5% duty cycle = audible tone at current frequency
#define beep_on   ledcWrite(LEDC_CH_BUZZER, 64)

// 0% duty = silence
#define beep_off  ledcWrite(LEDC_CH_BUZZER, 0)

// Set frequency dynamically (replaces: TIM3->ARR = 84000000/(f)/140-1)
// ESP32 LEDC uses ledcWriteTone for frequency changes
#define setBuzzerFrequency(f) ledcWriteTone(LEDC_CH_BUZZER, (f))

// =========================================================
//  Function prototypes
// =========================================================
void buzzer_Configuration(void);
void beep(int times);
void shortBeep(int duration, int freq);
void buzzerTick(void);   // Call this every 1ms to auto-shutoff
