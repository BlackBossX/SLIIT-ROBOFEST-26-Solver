#pragma once
#include <Arduino.h>

// =========================================================
//  robot_params.h — Shared parameter structure
//
//  This struct is shared between:
//    • Main ESP32  (robot) — reads these values
//    • Ground station ESP-C3 — sends updates via ESP-NOW
//
//  Keep this file identical in BOTH projects so the
//  ESP-NOW payload deserializes correctly.
// =========================================================

struct RobotParams {

    // =====================================================
    //  PID — Left Motor
    // =====================================================
    float pid_L_kp;     // Proportional gain    (try 1.0–5.0)
    float pid_L_ki;     // Integral gain        (try 0.0–0.5)
    float pid_L_kd;     // Derivative gain      (try 0.0–2.0)

    // =====================================================
    //  PID — Right Motor
    // =====================================================
    float pid_R_kp;
    float pid_R_ki;
    float pid_R_kd;

    // =====================================================
    //  PID — Wall-following (side error correction)
    // =====================================================
    float pid_W_kp;     // Wall Kp (try 0.5–3.0)
    float pid_W_ki;     // Wall Ki
    float pid_W_kd;     // Wall Kd

    // =====================================================
    //  Motor speeds (duty cycle 0–1023)
    // =====================================================
    int16_t speed_fwd;      // Normal forward speed      (0–1023)
    int16_t speed_turn;     // Turn speed (outer wheel)  (0–1023)
    int16_t speed_slow;     // Slow approach speed       (0–1023)

    // =====================================================
    //  Maze / navigation
    // =====================================================
    uint16_t cell_size_mm;      // Maze cell size in mm (standard = 180mm)
    uint16_t turn_ticks;        // Encoder ticks for a 90° turn
    uint16_t fwd_ticks;         // Encoder ticks for one cell forward

    // =====================================================
    //  Sensor thresholds (mm)
    // =====================================================
    uint16_t wall_front_thresh;   // Front wall detect distance (mm)
    uint16_t wall_side_thresh;    // Side  wall detect distance (mm)
    uint16_t wall_diag_thresh;    // Diagonal sensor threshold  (mm)

    // =====================================================
    //  Calibration
    // =====================================================
    int16_t enc_L_direction;   // +1 or -1 (flip if encoder counts backwards)
    int16_t enc_R_direction;   // +1 or -1
    uint8_t sensor_read_us;    // Delay between sensor reads (µs)
    uint8_t command_id;        // 0=idle, 1=fwd, 2=back, 3=left90, 4=right90, 5=left45, 6=right45, 7=turn180

};  // sizeof(RobotParams) must be ≤ 250 bytes for ESP-NOW

// Default values — applied at boot if no Ground Station
// has pushed an update yet.
inline RobotParams defaultParams() {
    RobotParams p;
    p.pid_L_kp = 2.0f;
    p.pid_L_ki = 0.0f;
    p.pid_L_kd = 0.5f;
    p.pid_R_kp = 2.0f;
    p.pid_R_ki = 0.0f;
    p.pid_R_kd = 0.5f;
    p.pid_W_kp = 1.5f;
    p.pid_W_ki = 0.0f;
    p.pid_W_kd = 0.3f;
    p.speed_fwd   = 400;
    p.speed_turn  = 350;
    p.speed_slow  = 200;
    p.cell_size_mm   = 180;
    p.turn_ticks     = 220;
    p.fwd_ticks      = 400;
    p.wall_front_thresh = 80;
    p.wall_side_thresh  = 90;
    p.wall_diag_thresh  = 120;
    p.enc_L_direction   = 1;
    p.enc_R_direction   = 1;
    p.sensor_read_us    = 20;
    p.command_id        = 0;
    return p;
}
