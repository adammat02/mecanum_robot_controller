# Mecanum Robot Controller

Firmware for a 4-wheel mecanum robot, based on the **STM32G474RET6** (Cortex-M4, 170 MHz, LQFP64).

> **Hardware:** This firmware runs on a custom PCB. The physical board design (schematic + layout) lives in the [mecanum_robot_board](https://github.com/adammat02/mecanum_robot_board) repository.

This is the board-side component of the [mecanum_robot_system](https://github.com/amatusia/mecanum_robot_system) project. It runs directly on the STM32 hardware and communicates with the host over USB CDC (Virtual COM Port).

## Hardware Overview

| Peripheral | Role |
|---|---|
| TIM1, TIM3, TIM4, TIM5 | Quadrature encoder interfaces (one per wheel) |
| TIM8 CH1–CH4 | PWM output for 4 motor drivers |
| I2C1, I2C2 | Sensor bus (IMU, etc.) |
| SPI1 | High-speed peripheral bus (42.5 Mbit/s) |
| USART1 | Debug / serial communication |
| USB | USB Full-Speed CDC (Virtual COM Port) |
| ADC1 | Analog input (battery voltage, current sense, etc.) |

## Timer Configuration

System clock: **170 MHz** (HSI × PLL, Voltage Scale 1 Boost).

| Timer | Role | Pins | PSC | Period | Frequency / notes |
|---|---|---|---|---|---|
| TIM1 | Encoder | PC0 / PC1 | 0 | 65 535 | 170 MHz tick, TI1 mode, filter=15 |
| TIM3 | Encoder | PB4 / PB5 | 0 | 65 535 | 170 MHz tick, TI2 mode, filter=15 |
| TIM4 | Encoder | PB6 / PB7 | 0 | 65 535 | 170 MHz tick, TI2 mode, filter=15 |
| TIM5 | Encoder | PA0 / PA1 | 0 | 4 294 967 295 | 170 MHz tick, TI1 mode, filter=15 |
| TIM2 | Microsecond timebase | — | 169 | 4 294 967 295 | **1 MHz** → 1 µs/tick, 32-bit free-running |
| TIM6 | ADC trigger | — | 16 999 | 999 | **10 Hz** TRGO → ADC1 |
| TIM8 | PWM — motor drivers | PC6–PC9 | 24 | 255 | **26.6 kHz**, 8-bit resolution (CCR 0–255) |

## Project Structure

```
Core/Src, Core/Inc          — HAL init code and application sources (CubeMX)
USB_Device/App/             — USB CDC interface (RX/TX buffers, callbacks)

Drivers/
  CMSIS/DSP/                — PID controller (arm_pid_init_f32, arm_pid_reset_f32)
  VL53L1X/core/             — VL53L1X ToF sensor API
  VL53L1X/platform/         — HAL I2C binding for VL53L1X
  STM32G4xx_HAL_Driver/     — STM32G4 HAL (CubeMX)

modules/
  comm/                     — UART communication and ASCII command parsing
  controller/               — High-level robot controller (orchestrates motors, PID, sensors)
  motors/                   — Motor driver, quadrature encoder, closed-loop RPM PID
  sensors/                  — Battery voltage (ADC) and VL53L1X ToF distance sensor

mecanum_robot_controller.ioc — CubeMX project file
CMakeLists.txt               — build entry point (user sources added here)
```

## USB CDC Communication

The board enumerates as a USB CDC device (Virtual COM Port). No baud rate configuration is needed — the USB link is always full-speed regardless of the line coding set by the host.

### Receiving data from the board

`printf()` is redirected to the USB CDC interface via `_write()` in [Core/Src/main.c](Core/Src/main.c). Any `printf` call in firmware will send text over USB to the host.

```c
printf("Hello from STM32\n");
```

### Sending data to the board

Incoming bytes are stored in `UserRxBufferFS` (2048 bytes). The `cdc_rx_ready` flag is set to `1` by the CDC receive callback when a packet arrives. Poll it in the main loop:

```c
if (cdc_rx_ready) {
    // UserRxBufferFS contains the received data
    cdc_rx_ready = 0;
}
```

### Sending raw bytes from firmware

Use `CDC_Transmit_FS` directly for binary data:

```c
uint8_t buf[] = {0x01, 0x02, 0x03};
CDC_Transmit_FS(buf, sizeof(buf));
```

The function returns `USBD_BUSY` if a previous transfer is still in progress.

## Modules

Custom application code lives in `modules/`. Each module has an `Inc/` directory with public headers and a `Src/` directory with implementations.

### comm

Line-based UART communication layer split into two components:

- **`uart_comm`** — interrupt-driven RX that buffers incoming bytes and signals when a complete line (`\n` / `\r`) is ready. Transmission is blocking via `uart_send_str()`.
- **`cmd_parser`** — parses ASCII lines into `Command` structs. Supported commands:

| Command | Format | Description |
|---|---|---|
| `S` | `S <s0> <s1> <s2> <s3>` | Set individual wheel speeds [RPM] |
| `F` | `F <s0> <s1> <s2> <s3>` | Full frame: set speeds and request telemetry response |
| `P` | `P <kp> <ki> <kd>` | Update PID gains at runtime |
| `E` | `E` | Request encoder positions |
| `R` | `R` | Reset encoder counters |

### controller

Top-level orchestration module. Owns and initialises all hardware handles defined in `ctrl_config.h` (4 motors, 4 encoders, 4 PID instances, battery, ToF sensor).

| Function | Description |
|---|---|
| `controller_init(bool debug)` | Initialise all sub-modules; `debug=true` enables periodic printf telemetry |
| `controller_poll()` | Non-blocking check for a new UART command; dispatches to the appropriate handler |
| `controller_update()` | Run one PID update step — call at a fixed rate from the main loop |
| `controller_adc_callback(hadc)` | Forward ADC conversion-complete interrupt to the battery module |

Static configuration constants (`ctrl_config.h`): `N_MOTORS=4`, `PER_REV=1940`, `MAX_RPM=160`, `KP=3.0 / KI=0.5 / KD=0.2`, EMA `ALPHA=0.5`, motor sign array `[1, -1, 1, -1]`.

### motors

Three sub-modules for closed-loop wheel control:

**`motor_driver`** — sets PWM duty cycle (0–255, mapped to TIM8 CH1–CH4) and direction via a GPIO pin.

**`encoder`** — reads a hardware quadrature timer (TIM1 / TIM3 / TIM4 / TIM5) and accumulates shaft rotations as a `float`. Call `encoder_get_rotations()` to sample; `encoder_reset()` to zero the counter.

**`motor_pid`** — closed-loop RPM controller built on the CMSIS-DSP `arm_pid_f32` implementation. Features:
- EMA low-pass filter on the measured speed (`alpha` configurable)
- Feed-forward term (`ff_gain = max_pwm / max_rpm`) for faster transient response
- Integrator anti-windup: integrator is frozen when PWM is saturated

### sensors

**`battery`** — measures supply voltage via ADC1, triggered by TIM6 at 10 Hz. Call `battery_get_voltage()` to read the last result [V]. The divider ratio is calibrated with `DIV_RATIO`.

**`tof_vl53l1x`** — wraps the ST VL53L1X API for single-zone distance measurement over I2C. After `tof_init()`, call `tof_get_distance()` to read the last measurement [mm]. The sensor can be power-cycled via the XSHUT GPIO using `tof_reset()`.

## Main Loop (`Core/Src/main.c`)

### Startup sequence

After HAL and peripheral init, three calls bring the application up:

```c
micros_tim_init(&htim2);      // bind TIM2 as 1 µs free-running counter
uart_init(&huart1);           // start interrupt-driven UART RX
controller_init(true);        // init motors, encoders, PID, sensors; debug=true enables printf telemetry
```

### Loop timing

The main loop is purely time-based — no RTOS. Three software timers are polled with `micros()`:

| Period | Task |
|---|---|
| 1 ms (`CTRL_PERIOD_US`) | `controller_update()` — run one PID step for all 4 wheels |
| 10 ms (`CMD_PERIOD_US`) | `controller_poll()` — check for a new UART command and dispatch it |
| 500 ms (`LED_PERIOD_US`) | Toggle the onboard LED (heartbeat) |

### HAL callbacks

| Callback | Forwarded to |
|---|---|
| `HAL_UART_RxCpltCallback` | `uart_rx_byte_callback()` — appends received byte to the line buffer |
| `HAL_ADC_ConvCpltCallback` | `controller_adc_callback()` → `battery_measure_callback()` — updates `vbat` |

`printf` is redirected to USB CDC via `_write()`.

## Development

The project is generated with **STM32CubeMX**. To modify peripheral configuration, open `mecanum_robot_controller.ioc` in CubeMX and regenerate — user code sections (`USER CODE BEGIN / END`) are preserved.

IDE: STM32CubeIDE or VS Code with the STM32 CMake + clangd setup (`.clangd` and `.vscode/` are included).
