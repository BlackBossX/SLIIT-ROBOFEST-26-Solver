/*
 * =================================================================
 *  Micromouse Sensor Test — ESP32 Port
 *  Original target : STM32F4 (Keil uVision)
 *  Ported target   : ESP32 (PlatformIO / Arduino framework)
 * =================================================================
 *
 *  To upload:   pio run --target upload
 *  To monitor:  pio device monitor       (or PlatformIO sidebar)
 *
 *  This file mirrors user/main.c from the original STM32 project.
 *  It initializes all peripherals and runs a sensor diagnostic loop.
 * =================================================================
 */

#include <Arduino.h>

// User libraries — all ported to Arduino/ESP32
#include "led.h"
#include "pwm.h"
#include "encoder.h"
#include "buzzer.h"
#include "sensor_Function.h"

// =========================================================
//  1ms hardware timer for buzzerTick()
//  Replaces SysTick_Handler from stm32f4xx_it.c
// =========================================================
hw_timer_t* g_timer = nullptr;

void IRAM_ATTR onTimer() {
    buzzerTick();   // Decrement buzzerTime every 1ms to auto-shutoff
}

// =========================================================
//  setup() — runs once at boot
//  Mirrors the initialization block in the original main()
// =========================================================
void setup() {
    // ----- Serial (replaces USART1 @ 9600) -----
    // Using 115200 baud — much faster and standard for ESP32
    Serial.begin(115200);
    delay(100);
    Serial.println("\r\n===  Micromouse Sensor Test  (ESP32 Port)  ===");

    // ----- LEDs and IR Emitters -----
    LED_Configuration();
    LED1_ON;   // Visual indicator: boot started

    // ----- Motor PWM (replaces TIM4_PWM_Init) -----
    Motor_Init();

    // ----- Quadrature Encoders (replaces Encoder_Configration) -----
    Encoder_Configuration();

    // ----- Buzzer (replaces buzzer_Configuration) -----
    buzzer_Configuration();

    // ----- ADC / Sensor setup -----
    sensor_Configuration();

    // ----- 1ms hardware timer for buzzer auto-shutoff -----
    g_timer = timerBegin(0, 80, true);          // Timer 0, 80MHz/80 = 1MHz tick
    timerAttachInterrupt(g_timer, &onTimer, true);
    timerAlarmWrite(g_timer, 1000, true);       // Alarm every 1000µs = 1ms
    timerAlarmEnable(g_timer);

    // ----- Startup beep (replaces shortBeep(2000, 8000)) -----
    shortBeep(200, 4000);   // 200ms beep at 4kHz
    delay(300);

    LED1_OFF;
    Serial.println("Init complete. Starting sensor loop...\r\n");
}

// =========================================================
//  loop() — runs repeatedly
//  Mirrors the while(1) block in the original main()
// =========================================================
void loop() {
    // Read all sensors (IR + gyro + voltage inside readSensor)
    readSensor();
    readGyro();
    readVolMeter();

    // Print telemetry — same format as original printf() line
    Serial.printf(
        "LF %d  RF %d  DL %d  DR %d  aSpeed %d  angle %d  voltage %d  lenc %d  renc %d\r\n",
        (int)LFSensor,
        (int)RFSensor,
        (int)DLSensor,
        (int)DRSensor,
        (int)aSpeed,
        (int)angle,
        (int)voltage,
        (int)getLeftEncCount(),
        (int)getRightEncCount()
    );

    // Drive both motors forward at PWM = 100 (same as original test)
    setLeftPwm(100);
    setRightPwm(100);

    delay(1000);  // replaces delay_ms(1000)
}
