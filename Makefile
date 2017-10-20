#------------------------------------------------------------------------------
# Firmware build
#
# Selectable build options
#------------------------------------------------------------------------------

# TODO: Move these into make targets
#TARGET_BOARD         ?= BOARD_RFD77201
TARGET_BOARD         ?= BOARD_PCA10040
#TARGET_BOARD         ?= BOARD_SHOE_SENSORv2

SOC_FAMILY           := nRF52
#SOC_FAMILY           := Simblee

USE_DFU              ?= "no"

# TODO: Pretty sure we can run everything on SDK 12. Try it!
NRF51_SDK_BASE       := C:/Users/georg/sdks/nRF51_SDK_8.1.0_b6ed55f
NRF51_SDK_VERSION    := 8
NRF52_SDK_BASE       := ../nRF5_SDK_12.3.0_d7731ad
NRF52_SDK_VERSION    := 12
SIMBLEE_BASE         := C:/Users/georg/sdks/Simblee_248

NRF51_SOFTDEVICE_HEX := C:\Users\georg\sdks\nRF51_SDK_8.1.0_b6ed55f\components\softdevice\s130\hex\s130_softdevice.hex
#NRF51_SOFTDEVICE_HEX := C:\Users\georg\sdks\nRF5_SDK_12.3.0_d7731ad\components\softdevice\s130\hex\s130_nrf51_2.0.1_softdevice.hex
NRF52_SOFTDEVICE_HEX := $(NRF52_SDK_BASE)/components/softdevice/s132/hex/s132_nrf52_3.0.0_softdevice.hex

GNU_INSTALL_ROOT := /usr/local
GNU_VERSION := 6.3.1
GNU_PREFIX := arm-none-eabi

#------------------------------------------------------------------------------
# Define relative paths to SDK components
#------------------------------------------------------------------------------

ifeq ($(SOC_FAMILY),Simblee)
$(info Building for Simblee)

SDK_BASE      := $(NRF51_SDK_BASE)
COMPONENTS    := $(SDK_BASE)/components

LINKER_SCRIPT := $(SIMBLEE_BASE)/variants/Simblee/linker_scripts/gcc/Simblee.ld

CXX_SOURCE_FILES += $(SIMBLEE_BASE)/libraries/SimbleeBLE/SimbleeBLE.cpp
CXX_SOURCE_FILES += $(SIMBLEE_BASE)/variants/Simblee/variant.cpp

C_SOURCE_FILES += $(COMPONENTS)/toolchain/system_nrf51.c
C_SOURCE_FILES += src/i2c.nrf51.c

INC_BOTH += -I$(SIMBLEE_BASE)/cores/arduino
INC_BOTH += -I$(SIMBLEE_BASE)/system/Simblee
INC_BOTH += -I$(SIMBLEE_BASE)/variants/Simblee
INC_PATHS += -I$(COMPONENTS)/softdevice/s110/headers
CXX_INC_PATHS += -I$(SIMBLEE_BASE)/system/Simblee/include
CXX_INC_PATHS += -I$(SIMBLEE_BASE)/system/CMSIS/CMSIS/Include

INC_PATHS += -I$(COMPONENTS)/drivers_nrf/twi_master/incubated/
C_SOURCE_FILES += $(COMPONENTS)/drivers_nrf/twi_master/incubated/twi_hw_master.c

INC_BOTH += -I$(COMPONENTS)/drivers_nrf/adc
INC_BOTH += -I$(COMPONENTS)/libraries/pstorage

INC_BOTH += -I$(COMPONENTS)/softdevice/s110/headers

LDFLAGS += -L$(SIMBLEE_BASE)/variants/Simblee
LIBS += -lSimbleeSystem -lSimblee -lSimbleeBLE -lSimbleeGZLL -lSimbleeForMobile -lSimbleeCOM

CFLAGS += -D NRF51
CFLAGS += -D S110
CFLAGS += -mcpu=cortex-m0
CFLAGS += -mfloat-abi=soft

CXXFLAGS += -MMD -mcpu=cortex-m0 -DF_CPU=16000000
CXXFLAGS += -DARDUINO=10801 -D__Simblee__

LDFLAGS += -mcpu=cortex-m0

ASMFLAGS += -D NRF51
ASMFLAGS += -D S110

# TODO: Replace this proprietary blob with Arduino-nRF5 or remove
ARDUINO_CORE = arduino_core/core.a

# Detect OS and use correct RFD Loader
ifeq ($(detected_OS),Windows)
    RFD_LOADER 		:= $(SIMBLEE_BASE)/RFDLoader.exe
endif
ifeq ($(detected_OS),Darwin)  # Mac OS X
    RFD_LOADER 		:= $(SIMBLEE_BASE)/RFDLoader_osx
endif
ifeq ($(detected_OS),Linux)
    RFD_LOADER 		:= $(SIMBLEE_BASE)/RFDLoader_linux
endif

endif

ifeq ($(SOC_FAMILY),nRF52)
$(info Building for nRF52)

SDK_BASE      := $(NRF52_SDK_BASE)
COMPONENTS    := $(SDK_BASE)/components
LINKER_SCRIPT := src/nrf52832.ld

ASM_SOURCE_FILES  += $(COMPONENTS)/toolchain/gcc/gcc_startup_nrf52.S

C_SOURCE_FILES += $(COMPONENTS)/toolchain/system_nrf52.c
C_SOURCE_FILES += src/i2c.nrf52.c

#INC_PATHS += -I$(SIMBLEE_BASE)/cores/arduino
#CXX_INC_PATHS += -I$(SIMBLEE_BASE)/cores/arduino

INC_BOTH += -Iconfig

INC_BOTH += -I$(COMPONENTS)/toolchain/cmsis/include
INC_BOTH += -I$(COMPONENTS)/toolchain

# Softdevice
INC_BOTH += -I$(COMPONENTS)/softdevice/s132/headers
INC_BOTH += -I$(COMPONENTS)/softdevice/s132/headers/nrf52

# SDK12 libs
INC_BOTH += -I$(COMPONENTS)/libraries/log
INC_BOTH += -I$(COMPONENTS)/libraries/log/src
INC_BOTH += -I$(COMPONENTS)/libraries/timer
INC_BOTH += -I$(COMPONENTS)/libraries/crc16
INC_BOTH += -I$(COMPONENTS)/libraries/experimental_section_vars
INC_BOTH += -I$(COMPONENTS)/drivers_nrf/saadc
INC_BOTH += -I$(COMPONENTS)/drivers_nrf/ppi
INC_BOTH += -I$(COMPONENTS)/drivers_nrf/timer
INC_BOTH += -I$(COMPONENTS)/drivers_nrf/rtc
INC_BOTH += -I$(COMPONENTS)/drivers_nrf/uart
INC_BOTH += -I$(COMPONENTS)/drivers_nrf/twi_master

C_SOURCE_FILES += $(COMPONENTS)/drivers_nrf/twi_master/nrf_drv_twi.c
C_SOURCE_FILES += $(COMPONENTS)/drivers_nrf/rtc/nrf_drv_rtc.c

INC_BOTH += -I$(COMPONENTS)/libraries/fds
INC_BOTH += -I$(COMPONENTS)/libraries/fstorage

COMMON_FLAGS += -DNRF52
COMMON_FLAGS += -DS132
COMMON_FLAGS += -DNRF52832
COMMON_FLAGS += -DNRF52832_XXAA
COMMON_FLAGS += -DNORDIC_SDK_VERSION=$(NRF52_SDK_VERSION)
COMMON_FLAGS += -DRAM_R1_BASE=0x20003000
COMMON_FLAGS += -DNRF_SD_BLE_API_VERSION=3
COMMON_FLAGS += -DARM_MATH_CM4
COMMON_FLAGS += -DNRF_LOG_USES_RTT=1
COMMON_FLAGS += -DF_CPU=64000000

# Enable CRCs in the fds library
COMMON_FLAGS += -DFDS_CRC_ENABLED

COMMON_FLAGS += -mcpu=cortex-m4
COMMON_FLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16

CXXFLAGS += -MMD

LDFLAGS += -mcpu=cortex-m4
LDFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16

endif # SOC_FAMILY

# TODO: Add nRF51 build for SD130

TEMPLATE_PATH := $(COMPONENTS)/toolchain/gcc
RBC_MESH      := rbc_mesh

# TODO: This is hardcoded on - any reason to keep this flag?
ifeq ($(USE_RBC_MESH_SERIAL), "yes")
  SERIAL_STRING := "_serial"
#	CFLAGS += -DRBC_MESH_SERIAL
endif
COMMON_FLAGS += -DRBC_MESH_SERIAL=1 #-DBSP_SIMPLE


ifdef JLINK_SN
  JLINK_SERIAL_NUMBER= --snr $(JLINK_SN)
endif

ifeq ($(USE_DFU), "yes")
  DFU_STRING="_dfu"
endif

ifneq ($(filter debug,$(MAKECMDGOALS)),)
  BUILD_TYPE := debug
else
  BUILD_TYPE ?= release
endif

OUTPUT_NAME := rbc_mesh$(SERIAL_STRING)$(DFU_STRING)_$(TARGET_BOARD)_$(SOC_FAMILY)_$(BUILD_TYPE)

#------------------------------------------------------------------------------
# Proceed cautiously beyond this point.  Little should change.
#------------------------------------------------------------------------------

export OUTPUT_NAME
export GNU_INSTALL_ROOT

MAKEFILE_NAME := $(MAKEFILE_LIST)
MAKEFILE_DIR := $(dir $(MAKEFILE_NAME) )

# echo suspend
ifeq ("$(VERBOSE)","1")
  NO_ECHO :=
else
  NO_ECHO := @
endif

# Toolchain commands
CC       := "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-gcc"
AS       := "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-as"
AR       := "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-ar" -r
LD       := "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-ld"
NM       := "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-nm"
OBJDUMP  := "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-objdump"
OBJCOPY  := "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-objcopy"
SIZE     := "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-size"
MK       := mkdir
RM       := rm -rf
CP       := cp
GENDAT   := ./gen_dat
GENZIP   := zip

BUILDMETRICS  := ./buildmetrics.py

# function for removing duplicates in a list
remduplicates = $(strip $(if $1,$(firstword $1) $(call remduplicates,$(filter-out $(firstword $1),$1))))

# source common to all targets

C_SOURCE_FILES += src/proximity.c src/battery.c src/shoe_accel.c src/power_manage.c \
	src/app_evt.c src/mesh_control.c bsp/bsp.c src/jostle_detect.c src/rtc.c
CXX_SOURCE_FILES += src/config.cpp src/main.cpp src/sensor.cpp src/app_cmd.cpp \
	src/scheduler.cpp src/heartbeat.cpp
C_SOURCE_FILES += $(COMPONENTS)/libraries/timer/app_timer.c
C_SOURCE_FILES += $(COMPONENTS)/libraries/crc16/crc16.c

C_SOURCE_FILES += $(RBC_MESH)/src/serial_handler_uart.c
C_SOURCE_FILES += $(RBC_MESH)/src/mesh_aci.c

ifeq ($(CLOCK_MASTER), "yes")
	COMMON_FLAGS += -D CLOCK_MASTER=1
endif

ifeq ($(USE_DFU), "yes")
	COMMON_FLAGS += -D MESH_DFU=1
endif


C_SOURCE_FILES += $(RBC_MESH)/src/radio_control.c
C_SOURCE_FILES += $(RBC_MESH)/src/rbc_mesh.c
C_SOURCE_FILES += $(RBC_MESH)/src/timer.c
C_SOURCE_FILES += $(RBC_MESH)/src/timer_scheduler.c
C_SOURCE_FILES += $(RBC_MESH)/src/timeslot.c
C_SOURCE_FILES += $(RBC_MESH)/src/trickle.c
C_SOURCE_FILES += $(RBC_MESH)/src/mesh_gatt.c
C_SOURCE_FILES += $(RBC_MESH)/src/transport_control.c
C_SOURCE_FILES += $(RBC_MESH)/src/fifo.c
C_SOURCE_FILES += $(RBC_MESH)/src/event_handler.c
C_SOURCE_FILES += $(RBC_MESH)/src/version_handler.c
C_SOURCE_FILES += $(RBC_MESH)/src/handle_storage.c
C_SOURCE_FILES += $(RBC_MESH)/src/mesh_packet.c
C_SOURCE_FILES += $(RBC_MESH)/src/rand.c

C_SOURCE_FILES += $(SDK_BASE)/external/segger_rtt/SEGGER_RTT.c
C_SOURCE_FILES += $(SDK_BASE)/external/segger_rtt/SEGGER_RTT_printf.c
C_SOURCE_FILES += $(SDK_BASE)/external/segger_rtt/RTT_Syscalls_GCC.c
C_SOURCE_FILES += $(COMPONENTS)/libraries/log/src/nrf_log_frontend.c
C_SOURCE_FILES += $(COMPONENTS)/libraries/log/src/nrf_log_backend_serial.c

C_SOURCE_FILES += $(COMPONENTS)/ble/common/ble_advdata.c
C_SOURCE_FILES += $(COMPONENTS)/softdevice/common/softdevice_handler/softdevice_handler.c
C_SOURCE_FILES += $(COMPONENTS)/drivers_nrf/gpiote/nrf_drv_gpiote.c
C_SOURCE_FILES += $(COMPONENTS)/drivers_nrf/common/nrf_drv_common.c
C_SOURCE_FILES += $(COMPONENTS)/drivers_nrf/uart/nrf_drv_uart.c
C_SOURCE_FILES += $(COMPONENTS)/drivers_nrf/clock/nrf_drv_clock.c
C_SOURCE_FILES += $(COMPONENTS)/drivers_nrf/saadc/nrf_drv_saadc.c
C_SOURCE_FILES += $(COMPONENTS)/drivers_nrf/hal/nrf_saadc.c
C_SOURCE_FILES += $(COMPONENTS)/libraries/util/app_util_platform.c
C_SOURCE_FILES += $(COMPONENTS)/libraries/util/app_error.c
C_SOURCE_FILES += $(COMPONENTS)/libraries/util/app_error_weak.c
C_SOURCE_FILES += $(COMPONENTS)/libraries/util/sdk_errors.c
C_SOURCE_FILES += $(COMPONENTS)/libraries/bsp/bsp_btn_ble.c
C_SOURCE_FILES += $(COMPONENTS)/libraries/button/app_button.c
C_SOURCE_FILES += $(COMPONENTS)/libraries/fds/fds.c
C_SOURCE_FILES += $(COMPONENTS)/libraries/fstorage/fstorage.c

vpath %.c $(C_PATHS)

# includes common to all targets

INC_BOTH += -Isrc
INC_BOTH += -I$(RBC_MESH)
INC_BOTH += -I$(RBC_MESH)/include
INC_BOTH += -Ibsp
INC_BOTH += -I$(SDK_BASE)/external/segger_rtt

INC_BOTH += -I$(COMPONENTS)/softdevice/common/softdevice_handler
INC_BOTH += -I$(COMPONENTS)/toolchain
INC_BOTH += -I$(COMPONENTS)/toolchain/gcc
INC_BOTH += -I$(COMPONENTS)/libraries/util
INC_BOTH += -I$(COMPONENTS)/libraries/timer
INC_BOTH += -I$(COMPONENTS)/libraries/bsp
INC_BOTH += -I$(COMPONENTS)/libraries/button
INC_BOTH += -I$(COMPONENTS)/ble/common
INC_BOTH += -I$(COMPONENTS)/drivers_nrf/common
INC_BOTH += -I$(COMPONENTS)/drivers_nrf/clock
INC_BOTH += -I$(COMPONENTS)/drivers_nrf/hal
INC_BOTH += -I$(COMPONENTS)/drivers_nrf/gpiote
INC_BOTH += -I$(COMPONENTS)/drivers_nrf/spi_slave
INC_BOTH += -I$(COMPONENTS)/drivers_nrf/delay
INC_BOTH += -I$(COMPONENTS)/device

# Includes that are shared between C and C++
INC_PATHS += $(INC_BOTH)
CXX_INC_PATHS += $(INC_BOTH)

OBJECT_DIRECTORY = _build$(JLINK_SN)
LISTING_DIRECTORY = $(OBJECT_DIRECTORY)
OUTPUT_BINARY_DIRECTORY = $(OBJECT_DIRECTORY)

# Sorting removes duplicates
BUILD_DIRECTORIES := $(sort $(OBJECT_DIRECTORY) $(OUTPUT_BINARY_DIRECTORY) $(LISTING_DIRECTORY) )

ifeq ($(BUILD_TYPE),debug)
  DEBUG_FLAGS += -DDEBUG=1 -g -O0 -ggdb -DJLINK_SN=$(JLINK_SN)

  # Generate assembly listings
  COMMON_FLAGS += -Wa,-adhln
else
  DEBUG_FLAGS += -D NDEBUG -O3
endif

# flags common to all targets
COMMON_FLAGS += $(DEBUG_FLAGS)
COMMON_FLAGS += -DBLE_STACK_SUPPORT_REQD
COMMON_FLAGS += -DSOFTDEVICE_PRESENT
COMMON_FLAGS += -D$(TARGET_BOARD)

COMMON_FLAGS += -Wall -Werror -Wshadow -Wno-unused-function -Wno-comment

## Flags that are shared between C, C++, and ASM
CFLAGS += $(COMMON_FLAGS)
CXXFLAGS += $(COMMON_FLAGS)
ASMFLAGS += $(COMMON_FLAGS) -mthumb

## C flags
CFLAGS += -mthumb -mabi=aapcs --std=gnu11
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing -fno-builtin

## CPP flags
CXXFLAGS += -g -Os -w -std=gnu++14 -ffunction-sections -fdata-sections -fno-rtti
CXXFLAGS += -fno-exceptions -fno-builtin -MMD -mthumb -fno-threadsafe-statics

CXX := "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-g++"

## Linker flags
LDFLAGS += -Xlinker -Map=$(LISTING_DIRECTORY)/$(OUTPUT_NAME).map
LDFLAGS += -mthumb -mabi=aapcs -L $(TEMPLATE_PATH) -T$(LINKER_SCRIPT)
LDFLAGS += $(DEBUG_FLAGS)
LDFLAGS += -Wl,--gc-sections
LDFLAGS += --specs=nano.specs -lc -lnosys

## Assembler flags
ASMFLAGS += -x assembler-with-cpp


C_SOURCE_FILE_NAMES = $(notdir $(C_SOURCE_FILES))
C_PATHS = $(call remduplicates, $(dir $(C_SOURCE_FILES) ) )
C_OBJECTS = $(addprefix $(OBJECT_DIRECTORY)/, $(C_SOURCE_FILE_NAMES:.c=.o) )

CXX_SOURCE_FILE_NAMES = $(notdir $(CXX_SOURCE_FILES))
CXX_PATHS = $(call remduplicates, $(dir $(CXX_SOURCE_FILES) ) )
CXX_OBJECTS = $(addprefix $(OBJECT_DIRECTORY)/, $(CXX_SOURCE_FILE_NAMES:.cpp=.o) )

ASM_SOURCE_FILE_NAMES = $(notdir $(ASM_SOURCE_FILES))
ASM_PATHS = $(call remduplicates, $(dir $(ASM_SOURCE_FILES) ))
ASM_OBJECTS = $(addprefix $(OBJECT_DIRECTORY)/, $(ASM_SOURCE_FILE_NAMES:.S=.o) )

TOOLCHAIN_BASE = $(basename $(notdir $(GNU_INSTALL_ROOT)))

vpath %.c $(C_PATHS)
vpath %.cpp $(CXX_PATHS)
vpath %.S $(ASM_PATHS)

OBJECTS = $(CXX_OBJECTS) $(C_OBJECTS) $(ASM_OBJECTS) $(ARDUINO_CORE)

all: $(BUILD_DIRECTORIES) $(OBJECTS) $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).elf echosize
	@echo "*****************************************************"
	@echo "build project:   $(OUTPUT_NAME)"
	@echo "build type:      $(BUILD_TYPE)"
	@echo "build with:      $(TOOLCHAIN_BASE)"
	@echo "build target:    $(TARGET_BOARD)"
	@echo "build directory: $(OBJECT_DIRECTORY)"
	@echo "build options    --"
	@echo "USE_DFU          $(USE_DFU)"
	@echo "build products   --"
	@echo "                 $(OUTPUT_NAME).elf"
	@echo "                 $(OUTPUT_NAME).hex"
	@echo "*****************************************************"

# Create build directories
$(BUILD_DIRECTORIES):
	$(MK) $@

# Create objects from C SRC files
$(OBJECT_DIRECTORY)/%.o: %.c
	@echo Compiling C file: $(notdir $<)
	$(NO_ECHO)$(CC) $(CFLAGS) $(INC_PATHS) \
	-c $< -o $@ > $(OUTPUT_BINARY_DIRECTORY)/$*.lst

# Create objects from C++ SRC files
$(OBJECT_DIRECTORY)/%.o: %.cpp
	@echo Compiling C++ file: $(notdir $<)
	$(NO_ECHO)$(CXX) $(CXXFLAGS) $(CXX_INC_PATHS) \
	-c $< -o $@ > $(OUTPUT_BINARY_DIRECTORY)/$*.lst

# Assemble files
$(OBJECT_DIRECTORY)/%.o: %.S
	@echo Assembling file: $(notdir $<)
	$(NO_ECHO)$(CC) $(ASMFLAGS) $(INC_PATHS) \
	-c -o $@ $< > $(OUTPUT_BINARY_DIRECTORY)/$*.lst

# Link
$(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).elf: $(BUILD_DIRECTORIES) $(OBJECTS)
	@echo Linking target: $(OUTPUT_NAME).elf
	$(NO_ECHO)$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).elf

# Create binary .bin file from the .elf file
$(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).bin: $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).elf
	@echo Preparing: $(OUTPUT_NAME).bin
	$(NO_ECHO)$(OBJCOPY) -O binary $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).elf $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).bin

# Create binary .hex file from the .elf file
$(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).hex: $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).elf
	@echo Preparing: $(OUTPUT_NAME).hex
	$(NO_ECHO)$(OBJCOPY) -O ihex $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).elf $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).hex

# Merge SoftDevice with app hex (only required for native nRF5x builds)
$(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME)_merged.hex: $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).hex
	@echo Preparing $(OUTPUT_NAME)_merged.hex
	$(NO_ECHO)mergehex -m $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).hex $(NRF52_SOFTDEVICE_HEX) -o $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME)_merged.hex


# Phonies:
.PHONY: debug
debug: all ;@:

.PHONY: release
release: all ;@:

.PHONY: echosize
echosize: $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).elf
	-@echo ""
	$(NO_ECHO)$(SIZE) $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).elf
	-@echo ""

# Flash Simblee devices
.PHONY: install_simblee
install_simblee: $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).hex
	@echo Installing: $(OUTPUT_NAME).hex
	$(NO_ECHO)$(RFD_LOADER) $(SERIAL_PORT) $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).hex

# Flash nRF52 devices (incl SoftDevice; eg for production)
.PHONY: install_nordic_full
install_nordic_full: $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME)_merged.hex
	@echo Installing: $(OUTPUT_NAME)_merged.hex
	$(NO_ECHO)nrfjprog --program $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME)_merged.hex --verify --chiperase -f NRF52 $(JLINK_SERIAL_NUMBER)
	$(NO_ECHO)nrfjprog -r -f NRF52 $(JLINK_SERIAL_NUMBER)

# Flash nrf52 softdevice
.PHONY: flash_softdevice_nrf52
flash_softdevice_nrf52:
	@echo Flashing: $(NRF52_SOFTDEVICE_HEX)
	nrfjprog --program $(NRF52_SOFTDEVICE_HEX) -f nrf52 --sectorerase $(JLINK_SERIAL_NUMBER)
	nrfjprog --reset -f nrf52 $(JLINK_SERIAL_NUMBER)

# Flash nRF52 devices
.PHONY: install_nordic
install_nordic: $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).hex
	@echo Installing: $(OUTPUT_NAME).hex
	$(NO_ECHO)nrfjprog --program $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).hex --verify --sectorerase -f NRF52 $(JLINK_SERIAL_NUMBER)
	$(NO_ECHO)nrfjprog -r -f NRF52 $(JLINK_SERIAL_NUMBER)

# Clean
.PHONY: clean
clean:
	@echo Cleaning build directory
	$(NO_ECHO)$(RM) $(BUILD_DIRECTORIES)

.PHONY: cleanobj
cleanobj:
	$(NO_ECHO)$(RM) $(BUILD_DIRECTORIES)/*.o

# Set device configuration via serial
.PHONY: configure
configure:
	./pyaci/configure_sensor.py $(SENSOR_CONFIGURATION_OPTIONS) -d $(SERIAL_PORT) $(SENSOR_ID)
	$(NO_ECHO)nrfjprog -r -f NRF52 $(JLINK_SERIAL_NUMBER)

# High level commands
.PHONY: nrf51
nrf51: all install_nordic_NRF51 configure

.PHONY: simblee
simblee: all install_simblee configure

.DEFAULT_GOAL:=nrf52
.PHONY: nrf52
nrf52: all install_nordic configure

# Reset the device
.PHONY: reset
reset:
	nrfjprog --reset -f $(SOC_FAMILY) $(JLINK_SERIAL_NUMBER)

# Erase the device
.PHONY: erase
erase:
	nrfjprog --eraseall -f $(SOC_FAMILY) $(JLINK_SERIAL_NUMBER)
