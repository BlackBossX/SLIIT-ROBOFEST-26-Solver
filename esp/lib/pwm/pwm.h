#pragma once
#include <Arduino.h>

// =========================================================
//  TB6612FNG Dual H-Bridge Motor Driver
//
//  Motor A = Left Motor
//    PWMA  → GPIO23  (PWM speed control)
//    AIN1  → GPIO18  (direction bit 1)
//    AIN2  → GPIO19  (direction bit 2)
//
//  Motor B = Right Motor
//    PWMB  → GPIO17  (PWM speed control)
//    BIN1  → GPIO4   (direction bit 1)
//    BIN2  → GPIO16  (direction bit 2)
//
//  TB6612FNG truth table:
//    AIN1=H, AIN2=L  → Forward
//    AIN1=L, AIN2=H  → Reverse
//    AIN1=L, AIN2=L  → Short-brake (fast stop)
//    AIN1=H, AIN2=H  → Short-brake (fast stop)
// =========================================================

// Pin assignments
#define PIN_PWMA   17
#define PIN_AIN1    4
#define PIN_AIN2   16
#define PIN_PWMB   23
#define PIN_BIN1   18
#define PIN_BIN2   19

// LEDC channels (ESP32 hardware PWM)
#define LEDC_CH_PWMA   0
#define LEDC_CH_PWMB   1

// =========================================================
//  Tunable PWM parameters
// =========================================================
#define MOTOR_PWM_FREQ   21000   // 21 kHz (inaudible switching frequency)
#define MOTOR_PWM_RES    10      // 10-bit resolution → 0–1023
#define MOTOR_PWM_MAX    1023    // Maximum duty cycle value

// =========================================================
//  Function prototypes
// =========================================================
void    Motor_Init(void);

// speed range: -MOTOR_PWM_MAX … +MOTOR_PWM_MAX
// Positive = forward, Negative = reverse, 0 = brake
void    setLeftPwm(int32_t speed);
void    setRightPwm(int32_t speed);

// Convenience macro — hard-brake both motors
#define turnMotorOff() { setLeftPwm(0); setRightPwm(0); }
