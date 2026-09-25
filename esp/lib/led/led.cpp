#include "led.h"

// =========================================================
//  Configure all LED and IR emitter pins as outputs
// =========================================================
void LED_Configuration(void) {
    // Status LEDs
    pinMode(PIN_LED1,   OUTPUT);
    pinMode(PIN_LED2,   OUTPUT);
    pinMode(PIN_LED3,   OUTPUT);
    pinMode(PIN_LED4,   OUTPUT);
    pinMode(PIN_LED5,   OUTPUT);

    // IR emitters
    pinMode(PIN_LF_EM,   OUTPUT);
    pinMode(PIN_RF_EM,   OUTPUT);
    pinMode(PIN_SIDE_EM, OUTPUT);

    // Start with everything off
    ALL_LED_OFF;
    LF_EM_OFF;
    RF_EM_OFF;
    SIDE_EM_OFF;
}
