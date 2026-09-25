#include "encoder.h"
#include <ESP32Encoder.h>

// =========================================================
//  Two encoder instances — replaces TIM2 (left) and TIM5 (right)
//  from the original STM32 code.
//  ESP32Encoder uses the PCNT hardware peripheral internally,
//  which supports quadrature decoding just like the STM32 timers.
// =========================================================
static ESP32Encoder encoderLeft;
static ESP32Encoder encoderRight;

// =========================================================
//  Encoder_Configuration — replaces Encoder_Configration()
//  (note: original had a typo 'Configration')
// =========================================================
void Encoder_Configuration(void) {
    // Enable the weak pull-up on encoder pins to prevent floating inputs
    ESP32Encoder::useInternalWeakPullResistors = UP;

    // Attach left encoder (replaces TIM2: PA15/PB3)
    encoderLeft.attachFullQuad(PIN_ENC_L_A, PIN_ENC_L_B);
    encoderLeft.setCount(0);

    // Attach right encoder (replaces TIM5: PA0/PA1)
    encoderRight.attachFullQuad(PIN_ENC_R_A, PIN_ENC_R_B);
    encoderRight.setCount(0);
}

// =========================================================
//  Getters and resetters — same API as original STM32 code
// =========================================================
int32_t getLeftEncCount(void) {
    return (int32_t)encoderLeft.getCount();
}

int32_t getRightEncCount(void) {
    return (int32_t)encoderRight.getCount();
}

void resetLeftEncCount(void) {
    encoderLeft.setCount(0);
}

void resetRightEncCount(void) {
    encoderRight.setCount(0);
}
