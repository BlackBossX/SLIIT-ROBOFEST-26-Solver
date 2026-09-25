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
void Display_Telemetry(uint16_t rawL, uint16_t rawF, uint16_t rawR, 
                       bool wL, bool wF, bool wR, 
                       float gyro, int32_t encL, int32_t encR);

// Helper function to read all values and update both Serial and OLED
void Update_UI(void);
