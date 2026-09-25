#include "pwm.h"

// =========================================================
//  Motor_Init — configure TB6612FNG direction pins and
//  LEDC channels for PWM speed control.
//  Call once in setup() before using setLeftPwm/setRightPwm.
// =========================================================
void Motor_Init(void) {
    // Direction GPIO pins — digital outputs
    pinMode(PIN_AIN1, OUTPUT);
    pinMode(PIN_AIN2, OUTPUT);
    pinMode(PIN_BIN1, OUTPUT);
    pinMode(PIN_BIN2, OUTPUT);

    // PWM channels via LEDC (Motor A and Motor B speed pins)
    ledcSetup(LEDC_CH_PWMA, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
    ledcSetup(LEDC_CH_PWMB, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
    ledcAttachPin(PIN_PWMA, LEDC_CH_PWMA);
    ledcAttachPin(PIN_PWMB, LEDC_CH_PWMB);

    // Start with motors braked (all direction pins LOW, PWM=0)
    setLeftPwm(0);
    setRightPwm(0);

    Serial.println("[PWM] TB6612FNG motors initialized");
}

// =========================================================
//  setLeftPwm — Motor A (Left motor)
//
//  TB6612FNG direction control:
//    Forward : AIN1=HIGH, AIN2=LOW,  PWMA = |speed|
//    Reverse : AIN1=LOW,  AIN2=HIGH, PWMA = |speed|
//    Brake   : AIN1=LOW,  AIN2=LOW,  PWMA = 0
// =========================================================
void setLeftPwm(int32_t speed) {
    // Clamp to valid range
    if (speed >  MOTOR_PWM_MAX) speed =  MOTOR_PWM_MAX;
    if (speed < -MOTOR_PWM_MAX) speed = -MOTOR_PWM_MAX;

    if (speed > 0) {                     // Forward
        digitalWrite(PIN_AIN1, HIGH);
        digitalWrite(PIN_AIN2, LOW);
        ledcWrite(LEDC_CH_PWMA, (uint32_t)speed);
    } else if (speed < 0) {              // Reverse
        digitalWrite(PIN_AIN1, LOW);
        digitalWrite(PIN_AIN2, HIGH);
        ledcWrite(LEDC_CH_PWMA, (uint32_t)(-speed));
    } else {                             // Brake (short-brake)
        digitalWrite(PIN_AIN1, LOW);
        digitalWrite(PIN_AIN2, LOW);
        ledcWrite(LEDC_CH_PWMA, 0);
    }
}

// =========================================================
//  setRightPwm — Motor B (Right motor)
// =========================================================
void setRightPwm(int32_t speed) {
    if (speed >  MOTOR_PWM_MAX) speed =  MOTOR_PWM_MAX;
    if (speed < -MOTOR_PWM_MAX) speed = -MOTOR_PWM_MAX;

    if (speed > 0) {                     // Forward
        digitalWrite(PIN_BIN1, HIGH);
        digitalWrite(PIN_BIN2, LOW);
        ledcWrite(LEDC_CH_PWMB, (uint32_t)speed);
    } else if (speed < 0) {              // Reverse
        digitalWrite(PIN_BIN1, LOW);
        digitalWrite(PIN_BIN2, HIGH);
        ledcWrite(LEDC_CH_PWMB, (uint32_t)(-speed));
    } else {                             // Brake (short-brake)
        digitalWrite(PIN_BIN1, LOW);
        digitalWrite(PIN_BIN2, LOW);
        ledcWrite(LEDC_CH_PWMB, 0);
    }
}
