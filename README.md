# Dependencies

 - [NRF 51 SDK 8.1.0](https://developer.nordicsemi.com/nRF51_SDK/nRF51_SDK_v8.x.x/nRF51_SDK_8.1.0_b6ed55f.zip)
 - Arduino IDE
 - Simblee platform for arduino

# Setup

 - Update Makefile with paths to the NRF SDK, and your serial board, and where arduino is installed.

# Compile and flash devices

Program a shoe sensor with id 1, turning off serial for power savings (Shoe & Teacher Pendant)

`make program SERIAL_PORT=/dev/cu.usbserial-FTZ86FTC SENSOR_CONFIGURATION_OPTIONS="--no-serial" TARGET_BOARD=BOARD_SHOE_SENSOR SENSOR_ID=1`

Program an area sensor with id 3, turning off serial for power savings (Area & Lesson Sensors):

`make program SERIAL_PORT=/dev/cu.usbserial-AI04QL7P SENSOR_CONFIGURATION_OPTIONS="--no-serial" TARGET_BOARD=BOARD_LESSON_TRACKER SENSOR_ID=3`

Program a listening device that doesn't sleep, and listens all the time. (Listener)

`make program SERIAL_PORT=/dev/cu.usbserial-DN00CSZ7 SENSOR_CONFIGURATION_OPTIONS="--no-sleep" TARGET_BOARD=BOARD_RFD77201 SENSOR_ID=51`

Program a master clock device that doesn't sleep, listens all the time, and broadcasts its clock signal at full power. (Mothernode)

`make program SERIAL_PORT=/dev/cu.usbserial-AI04QL7P SENSOR_CONFIGURATION_OPTIONS="--no-sleep" TARGET_BOARD=BOARD_LESSON_TRACKER CLOCK_MASTER=yes SENSOR_ID=61`
