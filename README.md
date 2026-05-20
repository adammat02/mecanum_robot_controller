# Mecanum Robot Controller

Firmware for a 4-wheel mecanum robot, based on the **STM32G474RET6** (Cortex-M4, 170 MHz, LQFP64).

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

## Development

The project is generated with **STM32CubeMX**. To modify peripheral configuration, open `mecanum_robot_controller.ioc` in CubeMX and regenerate — user code sections (`USER CODE BEGIN / END`) are preserved.

IDE: STM32CubeIDE or VS Code with the STM32 CMake + clangd setup (`.clangd` and `.vscode/` are included).
