# ESP32 Port — Micromouse Sensor Test

Port the STM32F4-based micromouse code to ESP32 using the **Arduino framework via PlatformIO**.  
The goal is to replicate the same test loop from `main.c`: initialize all peripherals, print sensor telemetry over Serial, and drive motors at a slow constant speed.

---

## Proposed Folder Structure

A new `esp/` folder will be created inside the `FuturaProject` root. It will be a **standalone PlatformIO project**.

```
FuturaProject/
└── esp/
    ├── platformio.ini          # PlatformIO build config
    ├── src/
    │   └── main.cpp            # Main program (replaces user/main.c)
    └── lib/
        ├── pwm/
        │   ├── pwm.h
        │   └── pwm.cpp
        ├── sensor/
        │   ├── sensor_Function.h
        │   └── sensor_Function.cpp
        ├── encoder/
        │   ├── encoder.h
        │   └── encoder.cpp
        ├── buzzer/
        │   ├── buzzer.h
        │   └── buzzer.cpp
        └── led/
            ├── led.h
            └── led.cpp
```

> **Note:** The Matrix Display (SPI-based) is omitted from this port — it is tightly coupled to a specific SPI display controller. You can add it back later once you select a compatible ESP32 display driver.

---

## STM32 → ESP32 Peripheral Mapping

| Feature | STM32 (Original) | ESP32 Equivalent |
|---|---|---|
| **Motor PWM** | TIM4 CH1-4 (PB6-9), 21 KHz | `ledcSetup` / `ledcAttachPin` (LEDC PWM), 21 KHz |
| **IR Sensors (ADC)** | ADC1 CH3,4,5,8,9 (PA3-5, PB0-1) | `analogRead()` on ADC1 pins |
| **Gyro (ADC)** | ADC1 CH15 | `analogRead()` on ADC1 pin |
| **Voltage ADC** | ADC1 CH3 | `analogRead()` |
| **Encoders** | TIM5 (quadrature, PA0/PA1), TIM2 (PA15/PB3) | `PCNT` pulse counter peripheral (hardware quadrature) |
| **Buzzer** | TIM3 CH1 PWM (PA6), 4 KHz | `ledcSetup` / `ledcAttachPin` (LEDC PWM) |
| **LEDs** | GPIO (multiple ports) | `digitalWrite()` on any free GPIO |
| **IR Emitters** | GPIO output (PC1, PC7, PA7) | `digitalWrite()` |
| **Serial (UART)** | USART1 @ 9600 baud | `Serial.begin(115200)` (`printf` → `Serial.printf`) |
| **SysTick (1ms)** | SysTick interrupt | `millis()` / `micros()` built-in |
| **Delay** | Custom `delay_ms` / `micros` | `delay()` / `micros()` built-in |

---

## Suggested ESP32 Pin Assignments

Since the original PCB is STM32-specific, these are **example/suggested pins** for an ESP32 DevKit. You will need to adjust these to match your actual ESP32 board wiring.

| Signal | ESP32 GPIO |
|---|---|
| **Left Motor Forward PWM** | GPIO 25 |
| **Left Motor Reverse PWM** | GPIO 26 |
| **Right Motor Forward PWM** | GPIO 27 |
| **Right Motor Reverse PWM** | GPIO 14 |
| **Left Front IR Emitter** | GPIO 32 |
| **Right Front IR Emitter** | GPIO 33 |
| **Side IR Emitter** | GPIO 4 |
| **LF IR Sensor (ADC)** | GPIO 34 (ADC1_CH6) |
| **RF IR Sensor (ADC)** | GPIO 35 (ADC1_CH7) |
| **DL IR Sensor (ADC)** | GPIO 36 (ADC1_CH0/VP) |
| **DR IR Sensor (ADC)** | GPIO 39 (ADC1_CH3/VN) |
| **Gyro Z-axis (ADC)** | GPIO 32 _(shared or separate)_ |
| **Battery Voltage (ADC)** | GPIO 35 _(shared or separate)_ |
| **Right Encoder CHA** | GPIO 18 |
| **Right Encoder CHB** | GPIO 19 |
| **Left Encoder CHA** | GPIO 22 |
| **Left Encoder CHB** | GPIO 23 |
| **Buzzer** | GPIO 2 |
| **LED1** | GPIO 5 |

> [!IMPORTANT]
> **GPIO 34, 35, 36, 39 on ESP32 are INPUT ONLY** — they cannot be used as outputs. They are ideal for ADC sensor inputs.
> **GPIO 12 must NOT be pulled HIGH at boot** — avoid using it for IR emitters.
> **GPIO 2 is the built-in LED** on most DevKit boards, which can double as a status LED.

---

## Key Implementation Details

### Motor PWM (LEDC)
- Uses 4 separate LEDC channels (one per motor direction pin).
- PWM resolution: **10-bit** (0–1023, matching the original 0–999 range).
- PWM frequency: **21 KHz**.

### IR Sensors
- The original uses a clever **ambient-light subtraction** technique:
  1. Read ADC with emitter OFF → baseline.
  2. Turn emitter ON → wait 60µs.
  3. Read ADC again → subtract baseline to get pure reflection.
- This logic is fully portable to `analogRead()` on ESP32.

### Encoders (PCNT)
- ESP32 has a dedicated **Pulse Counter (PCNT)** peripheral that supports hardware quadrature decoding — a direct equivalent to TIM2/TIM5 encoder mode.
- We will use the `ESP32Encoder` Arduino library (available in PlatformIO registry) for simplicity.

### Gyroscope
- The original reads a raw analog voltage from the gyro Z-axis and applies a calibration offset (`92980000`) with averaging.
- This translates directly to `analogRead()` with the same math.

### Serial Debug
- `printf()` is redirected to `Serial.printf()` via Arduino's built-in support.
- Baud rate changed to **115200** (standard for ESP32; 9600 is very slow).

---

## Files to Create

### `esp/platformio.ini`
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps =
    madhephaestus/ESP32Encoder @ ^0.10.1
```

### `esp/src/main.cpp`
Mirrors `user/main.c` using Arduino APIs.

### `esp/lib/pwm/pwm.h` & `pwm.cpp`
Motor control using LEDC.

### `esp/lib/sensor/sensor_Function.h` & `.cpp`
IR sensor reading with ambient subtraction, gyro averaging, and voltage reading.

### `esp/lib/encoder/encoder.h` & `.cpp`
Quadrature encoder reading using `ESP32Encoder` library.

### `esp/lib/buzzer/buzzer.h` & `.cpp`
Tone generation using LEDC.

### `esp/lib/led/led.h` & `.cpp`
GPIO-based LED and IR emitter control.

---

## Verification Plan

- Build the project with `pio run` and confirm **zero errors**.
- Upload with `pio run --target upload`.
- Monitor output with `pio device monitor` — confirm sensor values print correctly.
