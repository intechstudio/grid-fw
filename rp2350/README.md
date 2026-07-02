# rp2350

Minimal Raspberry Pi Pico SDK starter project targeting the **RP2350** (Pico 2).

Blinks the onboard LED and prints a heartbeat over USB CDC.

## Requirements

- Pico SDK **2.x** (RP2350 support). Point `PICO_SDK_PATH` at it, e.g.:
  ```sh
  export PICO_SDK_PATH=~/pico-sdk
  ```
- `arm-none-eabi-gcc` toolchain, CMake ≥ 3.13, `pico2` board (default in `CMakeLists.txt`).

## Build

```sh
cmake -S . -B build
cmake --build build -j
```

Output UF2: `build/main/main.uf2`.

## Flash

- **BOOTSEL**: hold BOOTSEL, plug in, copy `build/main/main.uf2` to the mass-storage volume.
- **OpenOCD/SWD** (see `rp2350-openocd.cfg` in this folder):
  ```sh
  openocd -f rp2350-openocd.cfg -c "program build/main/main.elf verify reset exit"
  ```

## Notes

- `PICO_BOARD pico2` selects the Arm Cortex-M33 build by default. For the RISC-V
  (Hazard3) cores, configure with `-DPICO_PLATFORM=rp2350-riscv`.
- This is a standalone starter — it does not link the shared `common/` sources
  used by the `rp2040` coprocessor firmware.
