# 🤖 Micromouse — Parameter Reference Guide

Every parameter here can be changed live from the **Ground Station web UI** and pushed to the robot via ESP-NOW (no re-flash needed).

---

## 📦 How Parameters Get to the Robot

```
Browser (Web UI)
  → HTTP POST /send  (JSON)
  → ESP32-C3 Ground Station
  → ESP-NOW radio packet
  → Robot ESP32 (RobotParams struct)
  → Applies instantly at next loop()
```

---

## 🏎️ Section: Hardware Tuner

These come directly from your `motor_encoder_tuner` and are the most important values to get right first.

| Parameter | Default | Unit | Description |
|---|---|---|---|
| `motor_l_trim` | `1.17` | multiplier | Scales **every** PWM value sent to the left motor. If your robot drifts right in open space, increase this slightly. If it drifts left, decrease it. |
| `motor_r_trim` | `1.0` | multiplier | Same as above but for the right motor. Usually left at 1.0 and only `motor_l_trim` is tuned. |
| `kp_bal` | `1.42` | — | **Encoder balance proportional gain.** Every loop it measures left vs. right wheel tick counts. Difference × `kp_bal` = PWM correction applied to both wheels. Higher = snappier straight driving but may oscillate. |
| `min_pwm` | `0` | 0–1023 | **Deadband / stiction floor.** Set to the lowest PWM that makes both motors spin (typically 50–100). Leave at 0 to disable. |
| `invert_flags` | `0` | bitmask | **Per-motor software inversion.** `bit 0 (1)` = flip Left motor, `bit 1 (2)` = flip Right motor, `bit 4 (16)` = swap Left/Right completely. Add bits together. Example: `3` = flip both motors. |

---

## 🧮 Section: PID — Motors (Unified)

> These are **currently reserved**. `kp_bal` (Hardware Tuner section) now controls straight-line balancing.

| Parameter | Default | Description |
|---|---|---|
| `pid_M_kp` | `2.0` | Reserved motor Kp |
| `pid_M_ki` | `0.0` | Reserved motor Ki |
| `pid_M_kd` | `0.5` | Reserved motor Kd |

---

## 🧱 Section: PID — Wall Following

Controls how aggressively the robot corrects during `moveForwardOneCell` using the 90° side sensors.

| Parameter | Default | Description |
|---|---|---|
| `pid_W_kp` | `1.5` | Wall Kp. Error = (sensor reading – 45mm). Higher = tighter wall hugging but may oscillate. |
| `pid_W_ki` | `0.0` | Reserved |
| `pid_W_kd` | `0.3` | Reserved |

---

## ⚡ Section: Motor Speeds

All values in 10-bit PWM units (0–1023).

| Parameter | Default | Description |
|---|---|---|
| `speed_fwd` | `400` | Base speed for forward/backward cell moves. PID corrections are added/subtracted around this value. |
| `speed_turn` | `350` | Speed for all spot turns (90°, 45°, 180°). Both wheels spin at this speed in opposite directions. |
| `speed_slow` | `200` | Reserved slow-approach speed for future deceleration near cell end. |

---

## 🗺️ Section: Maze Navigation

| Parameter | Default | Unit | Description |
|---|---|---|---|
| `cell_size_mm` | `180` | mm | Physical cell size. Documentation only — robot uses encoder ticks. |
| `fwd_ticks` | `400` | ticks | **Most critical navigation param.** Target encoder tick count for exactly one cell. Too small = stops short. Too large = overshoots. Tune by measuring physical travel. |
| `turn_ticks` | `220` | ticks | Reserved. Turns currently use gyro integration, not encoder ticks. |

---

## 📡 Section: Sensor Thresholds (mm)

| Parameter | Default | Description |
|---|---|---|
| `wall_front_thresh` | `80` | **Both** front sensors (0R + 0L) must read below this to detect a front wall (noise rejection). |
| `wall_side_thresh` | `90` | Side sensor (90R or 90L) threshold. Also the target distance for wall-following. |
| `wall_diag_thresh` | `120` | Reserved. Diagonal sensors (45R/45L) are read but not yet used in decisions. |

---

## 🔧 Section: Calibration

| Parameter | Default | Description |
|---|---|---|
| `enc_L_direction` | `+1` | Set to `-1` if left encoder counts down while going forward. |
| `enc_R_direction` | `+1` | Set to `-1` if right encoder counts down while going forward. |
| `sensor_read_us` | `20` | Reserved microsecond delay between sensor reads. |

---

## 🕹️ Manual Command Buttons

| Button | Command ID | What It Does |
|---|---|---|
| ▶ Start Auto | `9` | Enables autonomous maze-solving (Left-Hand Rule) |
| ■ Stop Auto | `8` | Disables autonomous mode, brakes motors |
| ▲ Forward | `1` | Drives exactly 1 cell forward (encoder-controlled) |
| ▼ Backward | `2` | Drives exactly 1 cell backward (encoder-controlled) |
| ↺ Turn 90° Left | `3` | Spot-turn left ~85° (gyro-controlled) |
| ↻ Turn 90° Right | `4` | Spot-turn right ~85° (gyro-controlled) |
| ↺ Turn 45° Left | `5` | Spot-turn left 45° |
| ↻ Turn 45° Right | `6` | Spot-turn right 45° |
| ↕ Turn 180° | `7` | U-turn 175° |

---

## 🔄 Fixing Turn Direction — `invert_flags` Guide

> [!IMPORTANT]
> Never edit code to fix turn direction. Use `invert_flags` from the web UI instead.

| Problem | Fix |
|---|---|
| Forward works, but both turns go opposite way | `invert_flags = 16` (swap motors) |
| Only left motor spins wrong way | `invert_flags = 1` |
| Only right motor spins wrong way | `invert_flags = 2` |
| Both individual motors reversed | `invert_flags = 3` |
| Left reversed + need to swap | `invert_flags = 17` |

> [!TIP]
> **Tuning order:** First set `invert_flags = 0`, test Forward. If robot goes backward → set `invert_flags = 3`. Then test Turn Left — if it goes right → set `invert_flags = 16`.

---

## 🏁 Recommended First-Time Tuning Order

1. **`invert_flags`** — get Forward and Turns physically correct first
2. **`fwd_ticks`** — tune so one cell = exactly 180mm
3. **`motor_l_trim`** — eliminate drift on open floor
4. **`kp_bal`** — tighten encoder balance (start low ~0.5, increase)
5. **`speed_fwd`** — increase once straight driving is solid
6. **`pid_W_kp`** — add wall-following tightness inside the maze
