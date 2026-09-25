# Maze Solver (Micromouse) Codebase Overview

This document provides a detailed breakdown of the FuturaProject codebase, which appears to be designed for an STM32F4-based Micromouse or maze-solving robot.

## Project Structure

The project is structured into several key directories:

- **`user/`**: Contains the main application entry point and interrupt service routines.
  - `main.c`: The core logic of the robot. Initializes peripherals and runs the main loop.
  - `stm32f4xx_it.c` / `.h`: Interrupt service routines for the STM32 microcontroller.
- **`user_Libraries/src/`**: Contains the source code (`.c` files) for various hardware peripherals and robot functions.
- **`user_Libraries/inc/`**: Contains the header files (`.h` files) for the libraries in `src/`.
- **`project/`**: Contains IDE-specific files, likely for Keil uVision (`.uvproj`, `.uvopt`), which is used to compile and flash the code to the STM32 microcontroller.
- **`ST_Libraries/`**: Standard Peripheral Libraries provided by STMicroelectronics for STM32F4.

## Key Components & Peripherals

The codebase abstracts hardware interactions into distinct modules. Here is a breakdown of the primary modules found in `user_Libraries/`:

### 1. Motor Control (`pwm.c`, `pwm.h`)
- **Timer Used**: `TIM4` (Timer 4) running at 84MHz.
- **Channels**: Uses all 4 channels of `TIM4` (Pins PB6, PB7, PB8, PB9) to generate PWM signals.
- **Frequency**: Configured to output a PWM frequency of 21KHz.
- **Functions**: 
  - `setLeftPwm(int32_t speed)` and `setRightPwm(int32_t speed)`.
  - The `speed` parameter ranges from -999 to +999. Positive values drive the motors forward, and negative values drive them in reverse.

### 2. Sensors (`sensor_Function.c`, `sensor_Function.h`)
This module handles reading data from the environment and internal state:
- **IR Sensors**: Reads values for Left Front (`LFSensor`), Right Front (`RFSensor`), Diagonal Left (`DLSensor`), and Diagonal Right (`DRSensor`). It toggles the IR emitters on and off to calculate the actual reflection value, compensating for ambient light.
- **Gyroscope**: `readGyro()` calculates the angular velocity (`aSpeed`) and integrates it to find the current heading `angle`.
- **Voltmeter**: `readVolMeter()` checks the battery voltage. `lowBatCheck()` ensures the robot stops and flashes a warning if the voltage drops below 7.0V to protect the battery.

### 3. Encoders (`encoder.c`, `encoder.h`)
- Tracks the rotation of the wheels to measure distance traveled and current speed.
- Provides functions like `getLeftEncCount()` and `getRightEncCount()` which are printed in the main loop for debugging.

### 4. User Interface & Feedback
- **Matrix Display (`matrixDisplay.c`)**: Used to display strings or integers on a small matrix screen (e.g., displaying "mous" or "Lbat" for low battery).
- **Buzzer (`buzzer.c`)**: Can emit tones, as seen in `main.c` (`shortBeep(2000, 8000);`) upon startup.
- **LEDs (`led.c`)**: Used for visual debugging and status indication.
- **Buttons (`button.c`)**: Handles user input to start/stop or change modes.

### 5. Communication & Debugging (`usart.c`, `usart.h`)
- Initializes UART communication (USART1) at a baud rate of 9600.
- Allows the use of `printf()` to send telemetry data back to a computer via a serial monitor (like the PlatformIO device monitor currently running).

## Program Execution Flow (`main.c`)

1. **Initialization**: The `main()` function begins by configuring all hardware peripherals:
   - System Tick (`Systick_Configuration()`)
   - LEDs, Buttons, Buzzer
   - Serial Communication (`usart1_Configuration(9600)`)
   - Sensors (ADC, SPI)
   - Motor PWM and Encoders
2. **Startup Signal**: Emits a `shortBeep` to indicate readiness.
3. **Main Loop (`while(1)`)**:
   - Reads all sensors (IR, Gyro, Battery Voltage).
   - Prints a telemetry string over UART with sensor values, speeds, angles, and encoder counts.
   - Displays "mous" on the matrix display.
   - Sets a constant PWM value of 100 to both left and right motors.
   - Delays for 1 second before repeating.

*Note: The current `main.c` appears to be a test script rather than a full maze-solving algorithm, as it simply reads sensors and drives forward at a slow, constant speed while logging data.*
