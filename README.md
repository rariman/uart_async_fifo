# UART Async FIFO Sample

This project is a Zephyr-based sample for the nRF54L15 DK that captures UART input asynchronously and stores each received line in a kernel FIFO queue. When the user presses the button, the queued data is dumped through the logging subsystem and the LED indicates activity.

## Overview

The application demonstrates:

- UART asynchronous reception using the Zephyr UART async API
- A FIFO queue to store received lines
- Button-triggered draining of queued UART data
- LED signaling for UART activity and FIFO dump events
- Zephyr logging for runtime diagnostics

## Hardware

- Board: nRF54L15 DK
- UART alias: `work-uart` mapped to `uart20`
- Button: `sw0`
- LEDs: `led0`, `led1`

The board mapping is defined in `nrf54l15dk_nrf54l15_cpuapp.overlay`.

## Features

- Receives UART bytes in non-blocking async mode
- Accumulates data until `\n` or `\r` is received
- Stores each complete line in a dynamically allocated FIFO item
- Disables UART RX while processing a line and re-enables it automatically
- Dumps queued lines when the button is pressed

## Prerequisites

Install the Nordic Semiconductor nRF Connect SDK / Zephyr toolchain and ensure these tools are available:

- `west`
- `cmake`
- `ninja` or `make`
- A C compiler for your host OS
- An nRF Connect SDK installation with the target board support

## Build

From the project root:

```bash
west build -p auto -b nrf54l15dk/nrf54l15/cpuapp
```

If using a different SDK setup, make sure the environment has `ZEPHYR_BASE` configured correctly.

## Flash

```bash
west flash
```

## Run

1. Connect the board to your computer.
2. Flash the firmware.
3. Open a serial terminal at the board's VCOM port with a baud rate of 115200.
4. Send text lines to the UART.
5. Press the configured button to dump the captured FIFO entries.

## Project Structure

```text
.
├── CMakeLists.txt
├── nrf54l15dk_nrf54l15_cpuapp.overlay
├── prj.conf
├── README.md
└── src/
    └── main.c
```

## Notes

- The application uses `CONFIG_UART_ASYNC_API=y` and `CONFIG_SERIAL=y` in `prj.conf`.
- The `HEAP_MEM_POOL_SIZE` is configured to allow dynamic allocation of queued UART items.
- Received lines are stored in memory until the button is pressed, which makes it useful for testing asynchronous UART buffering behavior.
