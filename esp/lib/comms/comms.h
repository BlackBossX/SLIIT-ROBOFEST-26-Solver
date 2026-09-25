#pragma once
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "../config/robot_params.h"

// =========================================================
//  comms.h — ESP-NOW Receiver (main ESP32 robot side)
//
//  The Ground Station (ESP-C3) broadcasts a RobotParams
//  struct. This library receives it and updates the live
//  parameter set on the robot.
//
//  Usage:
//    1. Call Comms_Init() in setup().
//    2. Call getParams() anywhere to read current params.
//    3. Call paramsUpdated() to check if a new set arrived.
// =========================================================

void          Comms_Init(void);          // Initialize ESP-NOW receiver
bool          paramsUpdated(void);       // True once after new data arrives
RobotParams&  getParams(void);           // Reference to live params
void          printParams(void);         // Dump params to Serial
