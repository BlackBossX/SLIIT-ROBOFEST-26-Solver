/*
 * ================================================================
 *  motion.cpp — Robot movement primitives
 *
 *  All motor output goes through setMotorsRaw() which applies:
 *    1. Software motor-swap  (invert_flags bit 4)
 *    2. Left / right trim    (motor_l_trim, motor_r_trim)
 *    3. Per-motor inversion  (invert_flags bit 0/1)
 *    4. Deadband mapping     (min_pwm)
 *
 *  Positive speed = forward for both motors.
 *  Turn convention: positive angle = Left, negative angle = Right.
 * ================================================================
 */

#include "motion.h"
#include "../encoder/encoder.h"
#include "../pwm/pwm.h"
#include "../gyro/gyro.h"
#include "../sensor/sensor_Function.h"
#include "../comms/comms.h"
#include "../display/display.h"

// ================================================================
//  setMotorsRaw — single motor-output gateway
//  All flags and trims applied here. Never call setLeftPwm /
//  setRightPwm directly from movement code.
// ================================================================
static void setMotorsRaw(int32_t speedL, int32_t speedR) {
    RobotParams& p = getParams();

    // 1. Software swap (bit 4)
    if (p.invert_flags & 0x10) {
        int32_t tmp = speedL; speedL = speedR; speedR = tmp;
    }

    // 2. Per-motor trim
    speedL = (int32_t)(speedL * p.motor_l_trim);
    speedR = (int32_t)(speedR * p.motor_r_trim);

    // 3. Per-motor inversion (bit 0 = left, bit 1 = right)
    if (p.invert_flags & 0x01) speedL = -speedL;
    if (p.invert_flags & 0x02) speedR = -speedR;

    // 4. Deadband: map 1..1023 → min_pwm..1023
    auto applyDeadband = [&](int32_t s) -> int32_t {
        if (s == 0 || p.min_pwm == 0) return s;
        int sgn = (s > 0) ? 1 : -1;
        int mag = map(abs((int)s), 1, 1023, p.min_pwm, 1023);
        return sgn * constrain(mag, (int)p.min_pwm, 1023);
    };
    speedL = applyDeadband(speedL);
    speedR = applyDeadband(speedR);

    setLeftPwm(speedL);
    setRightPwm(speedR);
}

// ================================================================
//  stopMotors
// ================================================================
void stopMotors() {
    setLeftPwm(0);
    setRightPwm(0);
    delay(150);
}

// ================================================================
//  moveForwardOneCell
//  Drives exactly p.fwd_ticks encoder ticks using:
//    • Encoder balance PID (straight assist)
//    • Wall-following correction layered on top
// ================================================================
void moveForwardOneCell() {
    RobotParams& p = getParams();

    resetLeftEncCount();
    resetRightEncCount();

    const int32_t  targetTicks   = p.fwd_ticks;
    int32_t        currentTicks  = 0;
    unsigned long  startTime     = millis();
    unsigned long  last_ui       = millis();

    while (currentTicks < targetTicks && (millis() - startTime) < 3000) {

        // UI refresh (50 ms)
        if (millis() - last_ui > 50) { Update_UI(); last_ui = millis(); }

        updateEncoders();
        int32_t left  = abs(getLeftEncCount()  * p.enc_L_direction);
        int32_t right = abs(getRightEncCount() * p.enc_R_direction);
        currentTicks  = (left + right) / 2;

        // --- Layer 1: Encoder balance (keeps both wheels at same tick count) ---
        float correction = (float)(left - right) * p.kp_bal;

        // --- Layer 2: Wall-following (blended on top of encoder balance) ---
        readSensors();
        bool wallL = isWallLeft(p.wall_side_thresh);
        bool wallR = isWallRight(p.wall_side_thresh);

        if (wallL && wallR) {
            // Centered between both walls
            float wallErr = (float)sensorMM[SENSOR_90L] - (float)sensorMM[SENSOR_90R];
            correction += wallErr * p.pid_W_kp;
        } else if (wallL) {
            // Hug left wall at 45 mm
            float wallErr = (float)sensorMM[SENSOR_90L] - 45.0f;
            correction += wallErr * p.pid_W_kp;
        } else if (wallR) {
            // Hug right wall at 45 mm
            float wallErr = 45.0f - (float)sensorMM[SENSOR_90R];
            correction -= wallErr * p.pid_W_kp;
        }

        // Positive correction → speed left motor up, slow right motor down
        int16_t pwm_l = constrain((int)(p.speed_fwd + correction), 0, 1023);
        int16_t pwm_r = constrain((int)(p.speed_fwd - correction), 0, 1023);

        setMotorsRaw(pwm_l, pwm_r);
        delay(5);
    }
    stopMotors();
}

// ================================================================
//  moveBackwardOneCell — encoder-assisted straight reverse
// ================================================================
void moveBackwardOneCell() {
    RobotParams& p = getParams();

    resetLeftEncCount();
    resetRightEncCount();

    const int32_t  targetTicks  = p.fwd_ticks;
    int32_t        currentTicks = 0;
    unsigned long  startTime    = millis();
    unsigned long  last_ui      = millis();

    while (currentTicks < targetTicks && (millis() - startTime) < 3000) {

        if (millis() - last_ui > 50) { Update_UI(); last_ui = millis(); }

        updateEncoders();
        int32_t left  = abs(getLeftEncCount()  * p.enc_L_direction);
        int32_t right = abs(getRightEncCount() * p.enc_R_direction);
        currentTicks  = (left + right) / 2;

        // Encoder balance (same sign convention, but applied to -speed)
        float correction = (float)(left - right) * p.kp_bal;

        int16_t pwm_l = constrain((int)(p.speed_fwd + correction), 0, 1023);
        int16_t pwm_r = constrain((int)(p.speed_fwd - correction), 0, 1023);

        // Reverse direction
        setMotorsRaw(-pwm_l, -pwm_r);
        delay(5);
    }
    stopMotors();
}

// ================================================================
//  turnByAngle — gyro-integrated spot turn
//
//  Convention (before invert_flags):
//    targetAngleDeg > 0 → robot turns LEFT  (L backward, R forward)
//    targetAngleDeg < 0 → robot turns RIGHT (L forward,  R backward)
//
//  If the physical robot turns the wrong way, use invert_flags bit 0
//  or bit 1 from the Ground Station to flip the offending motor.
// ================================================================
static void turnByAngle(float targetAngleDeg) {
    RobotParams& p = getParams();

    float         currentAngle = 0.0f;
    unsigned long lastTimeMicros = micros();
    unsigned long startTime    = millis();
    unsigned long last_ui      = millis();

    int16_t spd = p.speed_turn;

    // Standard turn command — setMotorsRaw applies all inversions
    if (targetAngleDeg > 0.0f) {
        // Turn Left: right wheel drives forward, left wheel drives backward
        setMotorsRaw(-spd, spd);
    } else {
        // Turn Right: left wheel drives forward, right wheel drives backward
        setMotorsRaw(spd, -spd);
    }

    while (abs(currentAngle) < abs(targetAngleDeg) && (millis() - startTime) < 2000) {

        if (millis() - last_ui > 50) { Update_UI(); last_ui = millis(); }

        unsigned long now = micros();
        float dt = (now - lastTimeMicros) / 1000000.0f;
        lastTimeMicros = now;

        currentAngle += getGyroZ() * dt;   // degrees
        delay(2);
    }
    stopMotors();
}

// ================================================================
//  Public turn wrappers
// ================================================================
void turnLeft90()    { turnByAngle( 85.0f); }
void turnRight90()   { turnByAngle(-85.0f); }
void turnLeft45()    { turnByAngle( 45.0f); }
void turnRight45()   { turnByAngle(-45.0f); }
void turnAround180() { turnByAngle(175.0f); }
