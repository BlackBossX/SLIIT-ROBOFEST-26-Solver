#include "buzzer.h"

// =========================================================
//  Buzzer time counter (decremented every 1ms by buzzerTick)
//  Mirrors the original STM32 'buzzerTime' global variable.
// =========================================================
int32_t buzzerTime = 0;

// =========================================================
//  buzzer_Configuration — replaces STM32 TIM3 PWM setup
//  On ESP32 we allocate one LEDC channel for the buzzer.
// =========================================================
void buzzer_Configuration(void) {
    // Resolution: 8-bit (0-255) is fine for a simple buzzer
    // We'll use ledcWriteTone() which overrides frequency dynamically
    ledcSetup(LEDC_CH_BUZZER, 4000, 8);   // default 4 KHz (buzzer resonance)
    ledcAttachPin(PIN_BUZZER, LEDC_CH_BUZZER);
    beep_off;   // Start silent
}

// =========================================================
//  beep — blocking beep, same as original
//  Emits 'times' short beeps of ~70ms on / 70ms off.
// =========================================================
void beep(int times) {
    buzzerTime = 140 * times;
    setBuzzerFrequency(4000);
    for (int i = 0; i < times; i++) {
        beep_on;
        delay(70);
        beep_off;
        delay(70);
    }
}

// =========================================================
//  shortBeep — non-blocking beep
//  Turns the buzzer on and sets buzzerTime; the buzzer will
//  auto-shutoff when buzzerTick() counts down to zero.
//  Call buzzerTick() from your 1ms timer or loop.
//
//  Replaces the original shortBeep() which relied on
//  SysTick_Handler decrementing buzzerTime every 1ms.
// =========================================================
void shortBeep(int duration, int freq) {
    setBuzzerFrequency(freq);
    beep_on;
    buzzerTime = duration;
}

// =========================================================
//  buzzerTick — call this every 1ms.
//  Mirrors the SysTick_Handler buzzer logic from stm32f4xx_it.c
// =========================================================
void buzzerTick(void) {
    if (buzzerTime > 0) {
        buzzerTime--;
    } else {
        beep_off;
    }
}
