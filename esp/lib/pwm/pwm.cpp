#include "pwm.h"

// =========================================================
//  Motor_Init — sets up 4 LEDC channels for motor control
//  Replaces TIM4_PWM_Init() from the original STM32 code
// =========================================================
void Motor_Init(void) {
    // Configure each LEDC channel with the same frequency/resolution
    ledcSetup(LEDC_CH_L_F, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
    ledcSetup(LEDC_CH_L_R, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
    ledcSetup(LEDC_CH_R_F, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
    ledcSetup(LEDC_CH_R_R, MOTOR_PWM_FREQ, MOTOR_PWM_RES);

    // Attach channels to physical GPIO pins
    ledcAttachPin(PIN_L_PWM_F, LEDC_CH_L_F);
    ledcAttachPin(PIN_L_PWM_R, LEDC_CH_L_R);
    ledcAttachPin(PIN_R_PWM_F, LEDC_CH_R_F);
    ledcAttachPin(PIN_R_PWM_R, LEDC_CH_R_R);

    // Start with motors stopped
    ledcWrite(LEDC_CH_L_F, 0);
    ledcWrite(LEDC_CH_L_R, 0);
    ledcWrite(LEDC_CH_R_F, 0);
    ledcWrite(LEDC_CH_R_R, 0);
}

// =========================================================
//  setLeftPwm — drives the left motor at a given speed
//  speed > 0 : forward
//  speed < 0 : reverse
//  range: -999 to +999  (same as original)
// =========================================================
void setLeftPwm(int32_t speed) {
    // Clamp input to valid range
    if (speed >  MOTOR_PWM_MAX) speed =  MOTOR_PWM_MAX;
    if (speed < -MOTOR_PWM_MAX) speed = -MOTOR_PWM_MAX;

    if (speed >= 0) {       // Forward
        ledcWrite(LEDC_CH_L_F, (uint32_t)speed);
        ledcWrite(LEDC_CH_L_R, 0);
    } else {                // Reverse
        ledcWrite(LEDC_CH_L_F, 0);
        ledcWrite(LEDC_CH_L_R, (uint32_t)(-speed));
    }
}

// =========================================================
//  setRightPwm — drives the right motor at a given speed
// =========================================================
void setRightPwm(int32_t speed) {
    if (speed >  MOTOR_PWM_MAX) speed =  MOTOR_PWM_MAX;
    if (speed < -MOTOR_PWM_MAX) speed = -MOTOR_PWM_MAX;

    if (speed >= 0) {       // Forward
        ledcWrite(LEDC_CH_R_F, (uint32_t)speed);
        ledcWrite(LEDC_CH_R_R, 0);
    } else {                // Reverse
        ledcWrite(LEDC_CH_R_F, 0);
        ledcWrite(LEDC_CH_R_R, (uint32_t)(-speed));
    }
}
