#pragma once
#include <Arduino.h>

// Initialize the MPU6050 on the I2C bus
bool Gyro_Init(void);

// Read and return the Z-axis rotation rate in degrees per second
float getGyroZ(void);
