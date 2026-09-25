# NETProject — Micromouse Solver

A complete ESP32-based Micromouse solving robot and ESP-C3 Ground Station.

## 🚀 Features

- **6x VL53L0X ToF Sensors**: High-speed, accurate distance measuring for wall detection and centering.
- **Hardware Encoders**: Uses ESP32 PCNT hardware to precisely track wheel rotation.
- **MPU6050 Gyro**: Integrates Z-axis rotation for perfect 90° and 180° turns without relying on wheel slip.
- **Graphical OLED UI**: Live 128x64 SSD1306 display showing a top-down view of the robot, detected walls, and live telemetry updating at 20fps!
- **ESP-NOW Ground Station**: A wireless remote-control Web UI hosted on a separate ESP-C3 to tune parameters live and send manual driving commands.
- **Wall Following**: The robot actively centers itself between walls using a Proportional controller.
- **Fail-safe Timeouts**: Movement loops have built-in timeouts so the robot won't soft-lock if picked up or stalled.

---

## ⚡ Pin Assignments (Main ESP32)

### I²C Bus (SDA: 21, SCL: 22)

_Shared by the ToF Sensors, Gyro, and OLED Display._

| Device / Sensor | Direction      | XSHUT GPIO  | I²C Address |
| --------------- | -------------- | ----------- | ----------- |
| 0               | 90° Right      | GPIO **13** | `0x30`      |
| 1               | 45° Right      | GPIO **14** | `0x31`      |
| 2               | 0° Front-Right | GPIO **25** | `0x32`      |
| 3               | 0° Front-Left  | GPIO **26** | `0x33`      |
| 4               | 45° Left       | GPIO **27** | `0x34`      |
| 5               | 90° Left       | GPIO **32** | `0x35`      |
| MPU6050 Gyro    | N/A            | N/A         | `0x68`      |
| SSD1306 OLED    | N/A            | N/A         | `0x3C`      |

> **Note:** The VL53L0X sensors boot up one by one using their XSHUT pins to assign unique addresses on the shared I²C bus.

### Encoders (Hardware PCNT)

| Signal          | GPIO   | Notes           |
| --------------- | ------ | --------------- |
| Left — Pulse A  | **34** | Input-only GPIO |
| Left — Ctrl B   | **36** | Input-only GPIO |
| Right — Pulse A | **39** | Input-only GPIO |
| Right — Ctrl B  | **35** | Input-only GPIO |

### Motor Driver (TB6612FNG)

| Signal | GPIO   | Function              |
| ------ | ------ | --------------------- |
| PWMA   | **23** | Left motor PWM        |
| AIN1   | **18** | Left direction bit 1  |
| AIN2   | **19** | Left direction bit 2  |
| PWMB   | **17** | Right motor PWM       |
| BIN1   | **4**  | Right direction bit 1 |
| BIN2   | **16** | Right direction bit 2 |

---

## 🎛️ Tunable Parameters

All parameters live in [`esp/lib/config/robot_params.h`](esp/lib/config/robot_params.h). They can be changed at compile time or at runtime from the Ground Station web UI. **Important:** The `robot_params.h` file must be kept identical between the Robot and Ground Station!

### PID Gains

| Parameter  | Default | Description                      |
| ---------- | ------- | -------------------------------- |
| `pid_M_kp` | `2.0`   | Unified Motor proportional gain  |
| `pid_M_ki` | `0.0`   | Unified Motor integral gain      |
| `pid_M_kd` | `0.5`   | Unified Motor derivative gain    |
| `pid_W_kp` | `1.5`   | Wall-following proportional gain |
| `pid_W_ki` | `0.0`   | Wall-following integral gain     |
| `pid_W_kd` | `0.3`   | Wall-following derivative gain   |

### Motor Speeds & Navigation

| Parameter           | Default | Description                                   |
| ------------------- | ------- | --------------------------------------------- |
| `speed_fwd`         | `400`   | Normal forward speed (0-1023)                 |
| `speed_turn`        | `350`   | Turn speed (0-1023)                           |
| `fwd_ticks`         | `400`   | Encoder ticks to travel one full cell (180mm) |
| `wall_front_thresh` | `80`    | Distance to detect front wall (mm)            |
| `wall_side_thresh`  | `90`    | Distance to detect side walls (mm)            |

---

## 📡 Ground Station Setup

The Ground Station runs on a separate **ESP-C3 Super Mini** and hosts a wireless configuration Web UI.

1. **Get the Robot's MAC**: Flash the robot and open the Serial Monitor. Copy the `MAC` address printed at boot.
2. **Update the Ground Station**: Open `esp/Ground Station/ground_c3.ino` in Arduino IDE. Set `ROBOT_MAC_ADDR` to the copied MAC.
3. **Flash the Ground Station**: Upload the code to your ESP-C3.
4. **Connect**: Join the `Micromouse-GS` Wi-Fi network on your phone/laptop (Password: `micromouse`).
5. **Tune**: Go to `http://192.168.4.1`. Change values and click "Send to Robot"!

---

## 📺 OLED & Serial Telemetry

While the robot runs, both the OLED screen and Serial Monitor update at high speed (20 FPS).

### Serial Output:

```text
[TEL] 90R= 250 45R= 180 0R=  72 0L=  68 45L= 195 90L= 260  encL=  1240 encR=  1238 GyroZ=0.0
[WALLS] Front=1 Right=0 Left=0
```

_(Note: `9999` means "Out of Range" / "No Wall" - this is normal behavior for open space!)_

### OLED Display:

The OLED shows a **Graphical Top-Down View** of the robot (a small square). When the sensors detect walls, thick white lines will appear around the robot on the screen to show exactly what it "sees" in real time!

---

## 🛠️ Testing & Debugging Mode

If you want to debug your sensors by moving your hand around the robot _without_ the motors spinning up and causing it to drive off the table:

1. Open `esp/src/main.cpp`
2. Find `bool test_sensors_only = false;` (around line 138)
3. Change it to `true` and upload!

---

## 📋 Dependency List (Robot)

| Library                            | Version  | Purpose      |
| ---------------------------------- | -------- | ------------ |
| `pololu/VL53L0X`                   | ^1.3.1   | ToF Sensors  |
| `adafruit/Adafruit MPU6050`        | ^2.2.6   | Gyro         |
| `adafruit/Adafruit Unified Sensor` | ^1.1.14  | Gyro         |
| `adafruit/Adafruit SSD1306`        | ^2.5.11  | OLED Display |
| `adafruit/Adafruit GFX Library`    | ^1.11.10 | OLED Drawing |
