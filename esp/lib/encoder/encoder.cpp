#include "encoder.h"

// =========================================================
//  32-bit software accumulators — PCNT hardware is 16-bit,
//  so we accumulate overflow events into these counters.
// =========================================================
static volatile int32_t leftAccum  = 0;
static volatile int32_t rightAccum = 0;

// =========================================================
//  PCNT overflow ISR — called when hardware counter wraps
// =========================================================
static void IRAM_ATTR pcnt_isr_handler(void* arg) {
    uint32_t status;
    pcnt_get_event_status(PCNT_UNIT_LEFT,  &status);
    // Left unit
    if (status & PCNT_EVT_H_LIM) leftAccum  += PCNT_HIGH_LIMIT;
    if (status & PCNT_EVT_L_LIM) leftAccum  += PCNT_LOW_LIMIT;

    pcnt_get_event_status(PCNT_UNIT_RIGHT, &status);
    // Right unit
    if (status & PCNT_EVT_H_LIM) rightAccum += PCNT_HIGH_LIMIT;
    if (status & PCNT_EVT_L_LIM) rightAccum += PCNT_LOW_LIMIT;
}

// =========================================================
//  Helper — configure one PCNT unit for quadrature decoding
// =========================================================
static void configurePCNT(pcnt_unit_t unit, int pulsePin, int ctrlPin) {
    pcnt_config_t cfg = {};
    cfg.pulse_gpio_num  = pulsePin;
    cfg.ctrl_gpio_num   = ctrlPin;
    cfg.unit            = unit;
    cfg.channel         = PCNT_CHANNEL_0;
    cfg.pos_mode        = PCNT_COUNT_INC;   // Rising edge on A → count up
    cfg.neg_mode        = PCNT_COUNT_DEC;   // Falling edge on A → count down
    cfg.lctrl_mode      = PCNT_MODE_REVERSE;// When B=LOW  → reverse direction
    cfg.hctrl_mode      = PCNT_MODE_KEEP;   // When B=HIGH → keep direction
    cfg.counter_h_lim   = PCNT_HIGH_LIMIT;
    cfg.counter_l_lim   = PCNT_LOW_LIMIT;
    pcnt_unit_config(&cfg);

    // Add second channel for full-quadrature (×4) decoding
    cfg.channel         = PCNT_CHANNEL_1;
    cfg.pulse_gpio_num  = ctrlPin;          // Swap: B is now the pulse source
    cfg.ctrl_gpio_num   = pulsePin;
    cfg.pos_mode        = PCNT_COUNT_DEC;
    cfg.neg_mode        = PCNT_COUNT_INC;
    pcnt_unit_config(&cfg);

    // Enable input filter to debounce noise (1.25µs filter at 80MHz APB clock)
    pcnt_set_filter_value(unit, 100);
    pcnt_filter_enable(unit);

    // Enable overflow events
    pcnt_event_enable(unit, PCNT_EVT_H_LIM);
    pcnt_event_enable(unit, PCNT_EVT_L_LIM);

    pcnt_counter_pause(unit);
    pcnt_counter_clear(unit);
    pcnt_counter_resume(unit);
}

// =========================================================
//  Encoder_Configuration — call once in setup()
// =========================================================
void Encoder_Configuration(void) {
    configurePCNT(PCNT_UNIT_LEFT,  ENC_L_PULSE_PIN, ENC_L_CTRL_PIN);
    configurePCNT(PCNT_UNIT_RIGHT, ENC_R_PULSE_PIN, ENC_R_CTRL_PIN);

    // Register single ISR for both units
    pcnt_isr_service_install(0);
    pcnt_isr_handler_add(PCNT_UNIT_LEFT,  pcnt_isr_handler, NULL);
    pcnt_isr_handler_add(PCNT_UNIT_RIGHT, pcnt_isr_handler, NULL);

    Serial.println("[ENC] Quadrature encoders configured (PCNT hardware)");
}

// =========================================================
//  updateEncoders — NOT required when using ISR overflow.
//  Kept as a no-op for API compatibility. The ISR handles
//  accumulation automatically.
// =========================================================
void updateEncoders(void) {
    // ISR handles overflow — nothing to do here.
}

// =========================================================
//  Getters — hardware count + software accumulator
// =========================================================
int32_t getLeftEncCount(void) {
    int16_t hw = 0;
    pcnt_get_counter_value(PCNT_UNIT_LEFT, &hw);
    return leftAccum + hw;
}

int32_t getRightEncCount(void) {
    int16_t hw = 0;
    pcnt_get_counter_value(PCNT_UNIT_RIGHT, &hw);
    return rightAccum + hw;
}

// =========================================================
//  Resetters
// =========================================================
void resetLeftEncCount(void) {
    pcnt_counter_clear(PCNT_UNIT_LEFT);
    leftAccum = 0;
}

void resetRightEncCount(void) {
    pcnt_counter_clear(PCNT_UNIT_RIGHT);
    rightAccum = 0;
}
