# 🐭 Micromouse — ESP32 Robot + ESP-C3 Ground Station

A complete firmware for a VL53L0X-based micromouse robot with wireless parameter tuning via an ESP32-C3 ground station.

---

## 📁 Project Structure

```
FuturaProject/
├── esp/                        ← Main robot firmware (ESP32)
│   ├── platformio.ini
│   ├── src/
│   │   └── main.cpp            ← Robot entry point
│   └── lib/
│       ├── config/
│       │   └── robot_params.h  ← Shared tunable parameters struct
│       ├── sensor/
│       │   ├── sensor_Function.h
│       │   └── sensor_Function.cpp   ← VL53L0X ToF sensor driver
│       ├── encoder/
│       │   ├── encoder.h
│       │   └── encoder.cpp           ← Raw PCNT quadrature decoder
│       ├── pwm/
│       │   ├── pwm.h
│       │   └── pwm.cpp               ← TB6612FNG motor driver
│       └── comms/
│           ├── comms.h
│           └── comms.cpp             ← ESP-NOW receiver
│
└── ground_station/             ← Ground Station firmware (ESP-C3)
    ├── platformio.ini
    ├── lib/config/
    │   └── robot_params.h      ← Same struct (must stay in sync)
    └── src/
        └── main.cpp            ← Web server + ESP-NOW sender
```

---

## ⚡ Pin Assignments

### Main ESP32 — Sensors (VL53L0X ToF, I²C)

| Sensor | Direction     | XSHUT GPIO | I²C Address |
|--------|---------------|-----------|-------------|
| 0      | 90° Right     | GPIO **13** | `0x30`    |
| 1      | 45° Right     | GPIO **14** | `0x31`    |
| 2      | 0° Front-Right| GPIO **25** | `0x32`    |
| 3      | 0° Front-Left | GPIO **26** | `0x33`    |
| 4      | 45° Left      | GPIO **27** | `0x34`    |
| 5      | 90° Left      | GPIO **32** | `0x35`    |

- **SDA** → GPIO 21  
- **SCL** → GPIO 22  
- All sensors share the **same I²C bus**. XSHUT is used to assign unique addresses at boot.

### Main ESP32 — Encoders (Hardware PCNT)

| Signal         | GPIO | Notes              |
|----------------|------|--------------------|
| Left — Pulse A | **34** | Input-only GPIO  |
| Left — Ctrl B  | **36** | Input-only GPIO  |
| Right — Pulse A| **39** | Input-only GPIO  |
| Right — Ctrl B | **35** | Input-only GPIO  |

> GPIO 34/35/36/39 are **input-only** on ESP32 — perfect for encoder inputs (no boot issues).

### Main ESP32 — Motor Driver (TB6612FNG)

| Signal | GPIO | Function             |
|--------|------|----------------------|
| PWMA   | **23** | Left motor PWM     |
| AIN1   | **18** | Left direction bit 1|
| AIN2   | **19** | Left direction bit 2|
| PWMB   | **17** | Right motor PWM    |
| BIN1   | **4**  | Right direction bit 1|
| BIN2   | **16** | Right direction bit 2|

**TB6612FNG direction truth table:**

| AIN1 | AIN2 | Motor A    |
|------|------|------------|
| HIGH | LOW  | Forward    |
| LOW  | HIGH | Reverse    |
| LOW  | LOW  | Short Brake|

### Built-in LED

| GPIO | Function |
|------|----------|
| **2** | Status LED — ON during boot, flashes on ESP-NOW receive |

---

## 🎛️ Tunable Parameters

All parameters live in [`lib/config/robot_params.h`](esp/lib/config/robot_params.h). They can be changed at compile time (defaults) or at runtime from the Ground Station web UI.

### PID Gains

| Parameter | Default | Description |
|-----------|---------|-------------|
| `pid_L_kp` | `2.0` | Left motor proportional gain |
| `pid_L_ki` | `0.0` | Left motor integral gain |
| `pid_L_kd` | `0.5` | Left motor derivative gain |
| `pid_R_kp` | `2.0` | Right motor proportional gain |
| `pid_R_ki` | `0.0` | Right motor integral gain |
| `pid_R_kd` | `0.5` | Right motor derivative gain |
| `pid_W_kp` | `1.5` | Wall-following proportional gain |
| `pid_W_ki` | `0.0` | Wall-following integral gain |
| `pid_W_kd` | `0.3` | Wall-following derivative gain |

> **Tuning tip:** Start with `Ki=0, Kd=0`. Increase `Kp` until oscillation, then add `Kd` to dampen.

### Motor Speeds (0 – 1023)

| Parameter | Default | Description |
|-----------|---------|-------------|
| `speed_fwd`  | `400` | Normal forward speed through a cell |
| `speed_turn` | `350` | Outer wheel speed during a 90° turn |
| `speed_slow` | `200` | Slow approach (near front wall) |

### Maze Navigation

| Parameter | Default | Description |
|-----------|---------|-------------|
| `cell_size_mm` | `180` | Standard micromouse cell = 180 mm |
| `fwd_ticks`    | `400` | Encoder ticks to travel one full cell |
| `turn_ticks`   | `220` | Encoder ticks for a 90° in-place turn |

> **Calibrating ticks:** Place robot at cell start, run forward one cell, read encoder count from Serial telemetry, enter that number here.

### Sensor Thresholds (mm)

| Parameter | Default | Description |
|-----------|---------|-------------|
| `wall_front_thresh` | `80`  | If front sensor < this → wall ahead |
| `wall_side_thresh`  | `90`  | If side sensor < this → wall on side |
| `wall_diag_thresh`  | `120` | Diagonal sensor threshold |

> Start conservatively high (120 mm) and reduce until the robot reliably detects walls before crashing.

### Calibration

| Parameter | Default | Description |
|-----------|---------|-------------|
| `enc_L_direction` | `+1` | Flip to `-1` if left encoder counts backwards |
| `enc_R_direction` | `+1` | Flip to `-1` if right encoder counts backwards |
| `sensor_read_us`  | `20` | Delay between consecutive sensor polls (µs) |

---

## 📡 Ground Station Setup

### Step 1 — Get robot MAC address
Flash and boot the robot. Open Serial monitor (`pio device monitor`). You'll see:
```
[COMMS] ESP-NOW ready. MAC: AB:CD:EF:01:23:45
```
Copy that MAC address.

### Step 2 — Set MAC in ground station firmware
Open [`ground_station/src/main.cpp`](ground_station/src/main.cpp) and set:
```cpp
uint8_t ROBOT_MAC_ADDR[6] = {0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45};
```

### Step 3 — Flash ground station
```bash
cd ground_station
pio run --target upload
```

### Step 4 — Connect and tune
1. On your phone or laptop, join Wi-Fi: **`Micromouse-GS`** / password: `micromouse`
2. Open browser → **`http://192.168.4.1`**
3. Edit any parameter and click **Send to Robot**
4. Robot's LED flashes to confirm receipt ✓
5. Robot Serial prints the received parameter dump

### ESP-NOW Technical Notes
- No router required — the C3 creates its own AP.
- Range: ~100 m line-of-sight outdoors, ~30 m indoors.
- Payload: `sizeof(RobotParams)` ≤ 250 bytes (ESP-NOW limit).
- If delivery fails, Serial prints `"Delivery FAILED"` — check the robot is powered and in range.

---

## 🔨 Build & Flash

### Robot (ESP32)
```bash
cd esp
pio run                      # Build only
pio run --target upload      # Flash
pio device monitor           # Open Serial monitor (115200 baud)
```

### Ground Station (ESP-C3)
```bash
cd ground_station
pio run --target upload
pio device monitor
```

> **ESP-C3 upload tip:** If upload fails, hold **BOOT** button, press **RST**, release **BOOT**, then retry.

---

## 🔎 Serial Telemetry (Robot)

Every loop the robot prints:
```
[TEL] 90R= 250 45R= 180 0R=  72 0L=  68 45L= 195 90L= 260  encL=  1240 encR=  1238
[WALLS] Front=1 Right=0 Left=0
```

| Field | Meaning |
|-------|---------|
| `90R/45R/0R/0L/45L/90L` | Distance in mm from each ToF sensor (`9999` = no reading) |
| `encL / encR`            | Cumulative left/right encoder tick count |
| `Front/Right/Left`       | `1` if a wall is detected on that side |

---

## ⚙️ Architecture Decisions

| Choice | Reason |
|--------|--------|
| **Raw PCNT** instead of ESP32Encoder library | Eliminates external dependency; uses hardware ISR overflow for 32-bit counting |
| **VL53L0X** over analog IR | ToF sensors give absolute mm distance, immune to ambient light and surface color |
| **XSHUT boot sequence** | All 6 VL53L0Xs share one I²C bus — XSHUT assigns unique addresses without a multiplexer |
| **TB6612FNG short-brake** (`AIN1=L, AIN2=L`) | Safest stop mode — back-EMF is clamped, motor decelerates quickly |
| **ESP-NOW broadcast** | No router, no IP config — works in a competition hall |
| **AsyncWebServer** | Non-blocking HTTP on single-core C3 — loop() stays free |

---

## 🗺️ What to Build Next

1. **PID speed controller** — use `getLeftEncCount()` / `getRightEncCount()` as feedback
2. **Wall-following** — use `sensorMM[SENSOR_90R]` vs `sensorMM[SENSOR_90L]` as error input to `pid_W_*`
3. **Maze solver** — implement Flood-Fill using the wall detection helpers
4. **Telemetry back to GS** — send `sensorMM[]` + encoder counts from robot to GS via ESP-NOW reply

---

## 📋 Dependency List

| Project | Library | Version |
|---------|---------|---------|
| esp (robot) | `pololu/VL53L0X` | ^1.3.1 |
| ground_station | `ESP Async WebServer` | latest |
| ground_station | `AsyncTCP` | latest |
| ground_station | `ArduinoJson` | ^6 |

All others use ESP-IDF drivers built into the `espressif32` platform (PCNT, esp_now, WiFi) — no extra installs needed.
