#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Initialize the OLED display on the shared I2C bus
bool Display_Init(void);

// Clear screen and print a simple message (used for boot)
void Display_Message(const char* msg);

// Update OLED with live telemetry (sensors, walls, etc.)
void Display_Telemetry(uint16_t wL, uint16_t wF, uint16_t wR, float gyro, int32_t encL, int32_t encR);
