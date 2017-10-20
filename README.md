# Dependencies

 - [NRF 52 SDK 12.3.0](https://www.nordicsemi.com/eng/nordic/download_resource/54291/56/98853373/32925)
 - [J-Link Software and Documentation Pack](https://www.segger.com/downloads/jlink/#J-LinkSoftwareAndDocumentationPack)
 - nrfjprog (on mac: `brew cask install nrf5x-command-line-tools`)
 - GCC ARM (on mac: `brew tap PX4/homebrew-px4 && brew install gcc-arm-none-eabi`)

# Setup

 - Install the NRF SDK at the same level as this project is checked out into.

# Compile and flash devices

Program a shoe sensor with id 1, turning off serial for power savings

`make program SERIAL_PORT=/dev/cu.usbserial-FTZ86FTC SENSOR_CONFIGURATION_OPTIONS="--no-serial" TARGET_BOARD=BOARD_SHOE_SENSOR SENSOR_ID=1`

Program an area sensor with id 3, turning off serial for power savings:

`make program SERIAL_PORT=/dev/cu.usbserial-AI04QL7P SENSOR_CONFIGURATION_OPTIONS="--no-serial" TARGET_BOARD=BOARD_LESSON_TRACKER SENSOR_ID=3`

Program a listening device that doesn't sleep, and listens all the time.

`make program SERIAL_PORT=/dev/cu.usbserial-DN00CSZ7 SENSOR_CONFIGURATION_OPTIONS="--no-sleep" TARGET_BOARD=BOARD_RFD77201 SENSOR_ID=51`

Program a master clock device that doesn't sleep, listens all the time, and broadcasts its clock signal at full power.

`make program SERIAL_PORT=/dev/cu.usbserial-AI04QL7P SENSOR_CONFIGURATION_OPTIONS="--no-sleep" TARGET_BOARD=BOARD_LESSON_TRACKER CLOCK_MASTER=yes SENSOR_ID=61`

# Debugging

- `JLinkGDBServer -if swd -device nrf52 -speed 4000`
- `arm-none-eabi-gdb _build/*.elf`
  - `target remote :2331`
  - `monitor reset`
  - `continue`
- `JLinkRTTClient`

# Building on Windows

- Make for windows (install via [chocolatey](https://chocolatey.org/packages/make))
- Simblee ([download](https://www.simblee.com/downloads/Simblee_248.tar.gz) and extract somewhere)
- GCC ARM Embedded Toolchain ([install](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads))

Paths should use forward slashes (e.g. `GNU_INSTALL_ROOT := C:/Program Files (x86)/GNU Tools ARM Embedded/6 2017-q2-update`)
