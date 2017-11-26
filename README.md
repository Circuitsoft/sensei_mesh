# Dependencies

 - [NRF 52 SDK 12.3.0](https://www.nordicsemi.com/eng/nordic/download_resource/54291/56/98853373/32925)
 - [J-Link Software and Documentation Pack](https://www.segger.com/downloads/jlink/#J-LinkSoftwareAndDocumentationPack)
 - nrfjprog (on mac: `brew cask install nrf5x-command-line-tools`)
 - GCC ARM (on mac: `brew tap PX4/homebrew-px4 && brew install gcc-arm-none-eabi`)

# Setup

 - Install the NRF SDK at the same level as this project is checked out into.

# Compile and flash devices

Program a shoe sensor with id 11, turning off serial for power savings:

`make install_nordic_full TARGET_BOARD=BOARD_SHOE_SENSORv2 && make configure SENSOR_CONFIGURATION_OPTIONS="--no-serial" SERIAL_PORT=/dev/cu.usbmodem1431 SENSOR_ID=11`

Program an area/lesson sensor with id 3, turning off serial for power savings:

`make install_nordic_full TARGET_BOARD=BOARD_LESSON_TRACKERv2 && make configure SENSOR_CONFIGURATION_OPTIONS="--no-serial" SERIAL_PORT=/dev/cu.SLAB_USBtoUART40 SENSOR_ID=3`

Program mothernode (area type sensor) that listens all the time and broadcasts time at high power.

`make install_nordic_full TARGET_BOARD=BOARD_LESSON_TRACKERv2 CLOCK_MASTER=yes && make configure SENSOR_CONFIGURATION_OPTIONS="--no-sleeping" SERIAL_PORT=/dev/cu.SLAB_USBtoUART SENSOR_ID=61`

# Debugging
- `make install_nordic_full debug -j9 DEBUG=1 VERBOSE=1 TARGET_BOARD=BOARD_SHOE_SENSORv2`
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
