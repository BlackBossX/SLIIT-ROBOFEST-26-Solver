#include "gyro.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

static Adafruit_MPU6050 mpu;
static float gyroZ_offset = 0.0;

bool Gyro_Init(void) {
    // Note: Wire.begin() is already called in Sensor_Configuration()
    if (!mpu.begin()) {
        Serial.println("[GYRO] ERROR: Failed to find MPU6050 chip");
        return false;
    }
    
    // Set typical micromouse gyro settings (high range, fast response)
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_44_HZ);
    
    Serial.println("[GYRO] MPU6050 Found! Calibrating Z-axis...");
    
    // Simple calibration (average a few readings while stationary)
    float sumZ = 0;
    int samples = 200;
    for (int i = 0; i < samples; i++) {
        sensors_event_t a, g, temp;
        mpu.getEvent(&a, &g, &temp);
        sumZ += g.gyro.z; // rad/s
        delay(5);
    }
    gyroZ_offset = sumZ / samples;
    Serial.printf("[GYRO] Z-axis offset: %.4f rad/s\n", gyroZ_offset);
    
    return true;
}

float getGyroZ(void) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    // Convert radians/s to degrees/s and remove static offset
    float z_rads = g.gyro.z - gyroZ_offset;
    return z_rads * 57.2958f; // rad to deg
}
