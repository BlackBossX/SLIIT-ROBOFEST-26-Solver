#include "motion.h"
#include "../encoder/encoder.h"
#include "../pwm/pwm.h"
#include "../gyro/gyro.h"
#include "../sensor/sensor_Function.h"
#include "../comms/comms.h"
#include "../display/display.h"

void stopMotors() {
    setLeftPwm(0);
    setRightPwm(0);
    delay(200); // Wait for robot to settle
}

void moveForwardOneCell() {
    RobotParams& p = getParams();
    
    // Reset encoders to start fresh for this cell
    resetLeftEncCount();
    resetRightEncCount();

    int32_t targetTicks = p.fwd_ticks;
    int32_t currentTicks = 0;
    
    unsigned long startTime = millis();
    unsigned long last_ui_update = millis();

    // Drive until the average encoder count reaches the target
    // Added a 3-second timeout in case encoders are disconnected or robot is stuck
    while (currentTicks < targetTicks && (millis() - startTime) < 3000) {
        if (millis() - last_ui_update > 50) {
            Update_UI();
            last_ui_update = millis();
        }

        updateEncoders();
        
        int32_t left = getLeftEncCount() * p.enc_L_direction;
        int32_t right = getRightEncCount() * p.enc_R_direction;
        // Use absolute values so backward-wired encoders don't break the target logic
        currentTicks = (abs(left) + abs(right)) / 2;
        
        // Use sensor readings for wall following
        readSensors();
        bool wallL = isWallLeft(p.wall_side_thresh);
        bool wallR = isWallRight(p.wall_side_thresh);
        
        float correction = 0.0f; // 0 if no walls
        
        if (wallL && wallR) {
            // Both walls present: Keep centered between them
            float wallError = (float)sensorMM[SENSOR_90L] - (float)sensorMM[SENSOR_90R];
            correction -= wallError * p.pid_W_kp;
        } else if (wallL) {
            // Only left wall: Follow left wall at a target distance of 45mm
            float wallError = (float)sensorMM[SENSOR_90L] - 45.0f;
            correction -= wallError * p.pid_W_kp;
        } else if (wallR) {
            // Only right wall: Follow right wall at a target distance of 45mm
            float wallError = 45.0f - (float)sensorMM[SENSOR_90R];
            correction -= wallError * p.pid_W_kp;
        }
        
        // Apply correction to base forward speed
        int16_t pwm_l = p.speed_fwd - (int16_t)correction;
        int16_t pwm_r = p.speed_fwd + (int16_t)correction;
        
        // Constrain PWM to safe bounds
        if (pwm_l > 1023) pwm_l = 1023;
        if (pwm_r > 1023) pwm_r = 1023;
        if (pwm_l < 0) pwm_l = 0;
        if (pwm_r < 0) pwm_r = 0;
        
        setLeftPwm(pwm_l);
        setRightPwm(pwm_r);
        delay(5);
    }
    stopMotors();
}

// Helper to turn using Gyro integration
static void turnByAngle(float targetAngleDeg) {
    RobotParams& p = getParams();
    float currentAngle = 0.0;
    unsigned long lastTime = micros();
    
    // Determine direction
    int16_t turnSpeed = p.speed_turn;
    if (targetAngleDeg < 0) {
        // Turn Right
        setLeftPwm(turnSpeed);
        setRightPwm(-turnSpeed);
    } else {
        // Turn Left
        setLeftPwm(-turnSpeed);
        setRightPwm(turnSpeed);
    }
    
    unsigned long startTime = millis();
    unsigned long last_ui_update = millis();

    // Integrate gyro Z over time until we reach the target angle
    // Added a 2-second timeout in case the robot is stuck or held in the air
    while (abs(currentAngle) < abs(targetAngleDeg) && (millis() - startTime) < 2000) {
        if (millis() - last_ui_update > 50) {
            Update_UI();
            last_ui_update = millis();
        }

        unsigned long now = micros();
        float dt = (now - lastTime) / 1000000.0f;
        lastTime = now;
        
        float gz = getGyroZ();
        currentAngle += gz * dt;
        
        delay(2);
    }
    stopMotors();
}

void turnLeft90() {
    // Usually a little less than 90 to account for momentum/overshoot
    turnByAngle(85.0); 
}

void turnRight90() {
    turnByAngle(-85.0);
}

void turnLeft45() {
    turnByAngle(45.0);
}

void turnRight45() {
    turnByAngle(-45.0);
}

void turnAround180() {
    turnByAngle(175.0);
}

void moveBackwardOneCell() {
    RobotParams& p = getParams();
    setLeftPwm(-p.speed_fwd);
    setRightPwm(-p.speed_fwd);
    delay(500); // Simple time-based reverse
    stopMotors();
}
