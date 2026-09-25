#pragma once
#include <Arduino.h>

// =========================================================
//  Motor PWM Pin Assignments   (adjust to match your wiring)
//  Each motor needs two pins: Forward and Reverse.
//  On an H-bridge like L298N or DRV8833 these map to IN1/IN2.
// =========================================================
#define PIN_L_PWM_F   25   // Left  motor forward
#define PIN_L_PWM_R   26   // Left  motor reverse
#define PIN_R_PWM_F   27   // Right motor forward
#define PIN_R_PWM_R   14   // Right motor reverse

// =========================================================
//  LEDC channel assignments  (ESP32 has 16 channels, 0-15)
// =========================================================
#define LEDC_CH_L_F   0
#define LEDC_CH_L_R   1
#define LEDC_CH_R_F   2
#define LEDC_CH_R_R   3

// =========================================================
//  PWM settings — mirrors the original STM32 21 KHz setup
//  Resolution: 10-bit (0-1023) ≈ original 0-999 range
// =========================================================
#define MOTOR_PWM_FREQ  21000   // 21 KHz, same as original
#define MOTOR_PWM_RES   10      // 10-bit resolution (0-1023)
#define MOTOR_PWM_MAX   999     // Maximum input value (match original)

// =========================================================
//  Function prototypes
// =========================================================
void     Motor_Init(void);
void     setLeftPwm(int32_t speed);
void     setRightPwm(int32_t speed);

#define  turnMotorOff   { setLeftPwm(0); setRightPwm(0); }
