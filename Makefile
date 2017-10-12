#------------------------------------------------------------------------------
# Firmware build
#
# Selectable build options
#------------------------------------------------------------------------------

TARGET_BOARD         ?= BOARD_RFD77201
#TARGET_BOARD         ?= BOARD_SHOE_SENSOR

USE_DFU              ?= "no"

#------------------------------------------------------------------------------
# Define relative paths to SDK components
#------------------------------------------------------------------------------

ifeq ($(shell uname),Linux)
	RFD_LOADER    = $(SIMBLEE_BASE)/RFDLoader_linux
	ARDUINO_BASE  := $(HOME)/.arduino15
	SERIAL_PORT   := /dev/ttyUSB0
else
	RFD_LOADER    = $(SIMBLEE_BASE)/RFDLoader_osx
	ARDUINO_BASE  := $(HOME)/Library/Arduino15
	#SERIAL_PORT  := /dev/cu.usbserial-DN00CSZ7  # left
	SERIAL_PORT   := /dev/cu.usbserial-DN00D34P  # right
	#SERIAL_PORT  := /dev/cu.usbserial-A105RB12
	#SERIAL_PORT  := /dev/cu.usbserial-FTZ86FTC  # tag-connect
	#SERIAL_PORT  := /dev/cu.usbserial-DO00C2G2  # Breadboard setup
endif

SDK_BASE      := ../nRF51_SDK_8.1.0_b6ed55f
COMPONENTS    := $(SDK_BASE)/components
TEMPLATE_PATH := $(COMPONENTS)/toolchain/gcc
SIMBLEE_BASE  := $(ARDUINO_BASE)/packages/Simblee/hardware/Simblee/1.1.2
RBC_MESH      := rbc_mesh

LINKER_SCRIPT := $(SIMBLEE_BASE)/variants/Simblee/linker_scripts/gcc/Simblee.ld

ifeq ($(USE_RBC_MESH_SERIAL), "yes")
	SERIAL_STRING := "_serial"
else
	SERIAL_STRING := ""
endif

ifeq ($(USE_DFU), "yes")
	DFU_STRING="_dfu"
else
	DFU_STRING=""
endif

OUTPUT_NAME := rbc_mesh$(SERIAL_STRING)$(DFU_STRING)_$(TARGET_BOARD)

GNU_INSTALL_ROOT := $(ARDUINO_BASE)/packages/Simblee/tools/arm-none-eabi-gcc/4.8.3-2014q1
GNU_PREFIX := arm-none-eabi

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

ifeq ($(MAKECMDGOALS),debug)
  BUILD_TYPE := debug
else
  BUILD_TYPE := release
endif

# Toolchain commands
CC       := "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-gcc"
CXX      := "$(GNU_INSTALL_ROOT)/bin/$(GNU_PREFIX)-g++"
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

C_SOURCE_FILES += src/main.c src/config.c src/sensor.c src/app_cmd.c \
 	src/scheduler.c src/proximity.c src/heartbeat.c src/battery.c src/shoe_accel.c \
	src/app_evt.c src/mesh_control.c bsp/bsp.c src/i2c.c src/jostle_detect.c
C_SOURCE_FILES += $(COMPONENTS)/libraries/timer/app_timer.c

CXX_SOURCE_FILES += $(SIMBLEE_BASE)/libraries/SimbleeBLE/SimbleeBLE.cpp
CXX_SOURCE_FILES += $(SIMBLEE_BASE)/variants/Simblee/variant.cpp

CFLAGS += -DRBC_MESH_SERIAL=1 -DBSP_SIMPLE
C_SOURCE_FILES += $(RBC_MESH)/src/serial_handler_uart.c
C_SOURCE_FILES += $(RBC_MESH)/src/mesh_aci.c

ifeq ($(CLOCK_MASTER), "yes")
	CFLAGS += -D CLOCK_MASTER=1
endif

ifeq ($(USE_DFU), "yes")
	CFLAGS += -D MESH_DFU=1
	C_SOURCE_FILES += $(RBC_MESH)/src/dfu_app.c
	C_SOURCE_FILES += $(RBC_MESH)/src/mesh_flash.c
	C_SOURCE_FILES += $(RBC_MESH)/src/nrf_flash.c
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

C_SOURCE_FILES += $(COMPONENTS)/ble/common/ble_advdata.c
C_SOURCE_FILES += $(COMPONENTS)/toolchain/system_nrf51.c
C_SOURCE_FILES += $(COMPONENTS)/softdevice/common/softdevice_handler/softdevice_handler.c
C_SOURCE_FILES += $(COMPONENTS)/drivers_nrf/twi_master/incubated/twi_hw_master.c
C_SOURCE_FILES += $(COMPONENTS)/drivers_nrf/gpiote/nrf_drv_gpiote.c
C_SOURCE_FILES += $(COMPONENTS)/drivers_nrf/common/nrf_drv_common.c

# assembly files common to all targets
#ASM_SOURCE_FILES  += $(COMPONENTS)/toolchain/gcc/gcc_startup_nrf51.s
LDFLAGS += -L$(SIMBLEE_BASE)/variants/Simblee
LIBS += -lSimbleeSystem -lSimblee -lSimbleeBLE -lSimbleeGZLL -lSimbleeForMobile -lSimbleeCOM
vpath %.c $(C_PATHS)

ARDUINO_CORE = arduino_core/core.a

# includes common to all targets

INC_PATHS += -Isrc
INC_PATHS += -I$(RBC_MESH)
INC_PATHS += -I$(RBC_MESH)/include
INC_PATHS += -Ibsp
INC_PATHS += -I../../../RTT
INC_PATHS += -I$(SIMBLEE_BASE)/cores/arduino
INC_PATHS += -I$(SIMBLEE_BASE)/system/Simblee
INC_PATHS += -I$(SIMBLEE_BASE)/variants/Simblee

INC_PATHS += -I$(COMPONENTS)/softdevice/s110/headers
INC_PATHS += -I$(COMPONENTS)/softdevice/common/softdevice_handler
INC_PATHS += -I$(COMPONENTS)/toolchain/gcc
INC_PATHS += -I$(COMPONENTS)/libraries/util
INC_PATHS += -I$(COMPONENTS)/libraries/timer
INC_PATHS += -I$(COMPONENTS)/ble/common
INC_PATHS += -I$(COMPONENTS)/drivers_nrf/common
INC_PATHS += -I$(COMPONENTS)/drivers_nrf/hal
INC_PATHS += -I$(COMPONENTS)/drivers_nrf/pstorage
INC_PATHS += -I$(COMPONENTS)/drivers_nrf/twi_master/incubated/
INC_PATHS += -I$(COMPONENTS)/drivers_nrf/gpiote

INC_PATHS += -I$(COMPONENTS)/toolchain/gcc
INC_PATHS += -I$(COMPONENTS)/toolchain
INC_PATHS += -I$(COMPONENTS)/device
INC_PATHS += -I$(COMPONENTS)/softdevice/s110/headers
INC_PATHS += -I$(COMPONENTS)/drivers_nrf/hal
INC_PATHS += -I$(COMPONENTS)/drivers_nrf/spi_slave

CXX_INC_PATHS += -I$(SIMBLEE_BASE)/cores/arduino
CXX_INC_PATHS += -I$(SIMBLEE_BASE)/variants/Simblee
CXX_INC_PATHS += -I$(SIMBLEE_BASE)/system/Simblee
CXX_INC_PATHS += -I$(SIMBLEE_BASE)/system/Simblee/include
CXX_INC_PATHS += -I$(SIMBLEE_BASE)/system/CMSIS/CMSIS/Include

OBJECT_DIRECTORY = _build
LISTING_DIRECTORY = $(OBJECT_DIRECTORY)
OUTPUT_BINARY_DIRECTORY = $(OBJECT_DIRECTORY)

# Sorting removes duplicates
BUILD_DIRECTORIES := $(sort $(OBJECT_DIRECTORY) $(OUTPUT_BINARY_DIRECTORY) $(LISTING_DIRECTORY) )

ifeq ($(BUILD_TYPE),debug)
  DEBUG_FLAGS += -D DEBUG -g -O0
else
  DEBUG_FLAGS += -D NDEBUG -O3
endif

# flags common to all targets
#CFLAGS += -save-temps
CFLAGS += $(DEBUG_FLAGS)
CFLAGS += -D NRF51
CFLAGS += -D BLE_STACK_SUPPORT_REQD
CFLAGS += -D S110
CFLAGS += -D SOFTDEVICE_PRESENT
CFLAGS += -D $(TARGET_BOARD)
CFLAGS += -mcpu=cortex-m0
CFLAGS += -mthumb -mabi=aapcs --std=gnu99
CFLAGS += -Wall -Werror
CFLAGS += -Wa,-adhln
CFLAGS += -mfloat-abi=soft
CFLAGS += -ffunction-sections
CFLAGS += -fdata-sections -fno-strict-aliasing
CFLAGS += -fno-builtin

# -c -g -Os -w -std=gnu++11 -ffunction-sections -fdata-sections -fno-rtti -fno-exceptions -fno-builtin -MMD -mcpu=cortex-m0
# -DF_CPU=16000000 -DARDUINO=10801 -D__PROJECT__="ledtest.ino" -mthumb -D__Simblee__

CXXFLAGS += -g -Os -w -std=gnu++11 -ffunction-sections -fdata-sections -fno-rtti
CXXFLAGS += -fno-exceptions -fno-builtin -MMD -mcpu=cortex-m0 -DF_CPU=16000000
CXXFLAGS += -DARDUINO=10801 -mthumb -D__Simblee__

LDFLAGS += -Xlinker -Map=$(LISTING_DIRECTORY)/$(OUTPUT_NAME).map
LDFLAGS += -mthumb -mabi=aapcs -L $(TEMPLATE_PATH) -T$(LINKER_SCRIPT)
LDFLAGS += -mcpu=cortex-m0
LDFLAGS += $(DEBUG_FLAGS)
LDFLAGS += -Wl,--gc-sections
LDFLAGS += --specs=nano.specs -lc -lnosys

# Assembler flags
ASMFLAGS += $(DEBUG_FLAGS)
ASMFLAGS += -x assembler-with-cpp
ASMFLAGS += -D NRF51
ASMFLAGS += -D BLE_STACK_SUPPORT_REQD
ASMFLAGS += -D S110
ASMFLAGS += -D SOFTDEVICE_PRESENT
ASMFLAGS += -D $(TARGET_BOARD)

C_SOURCE_FILE_NAMES = $(notdir $(C_SOURCE_FILES))
C_PATHS = $(call remduplicates, $(dir $(C_SOURCE_FILES) ) )
C_OBJECTS = $(addprefix $(OBJECT_DIRECTORY)/, $(C_SOURCE_FILE_NAMES:.c=.o) )

CXX_SOURCE_FILE_NAMES = $(notdir $(CXX_SOURCE_FILES))
CXX_PATHS = $(call remduplicates, $(dir $(CXX_SOURCE_FILES) ) )
CXX_OBJECTS = $(addprefix $(OBJECT_DIRECTORY)/, $(CXX_SOURCE_FILE_NAMES:.cpp=.o) )

ASM_SOURCE_FILE_NAMES = $(notdir $(ASM_SOURCE_FILES))
ASM_PATHS = $(call remduplicates, $(dir $(ASM_SOURCE_FILES) ))
ASM_OBJECTS = $(addprefix $(OBJECT_DIRECTORY)/, $(ASM_SOURCE_FILE_NAMES:.s=.o) )

TOOLCHAIN_BASE = $(basename $(notdir $(GNU_INSTALL_ROOT)))

TIMESTAMP := $(shell date +'%s')

vpath %.c $(C_PATHS)
vpath %.cpp $(CXX_PATHS)
vpath %.s $(ASM_PATHS)

OBJECTS = $(CXX_OBJECTS) $(C_OBJECTS) $(ASM_OBJECTS) $(ARDUINO_CORE)

all: $(BUILD_DIRECTORIES) $(OBJECTS)
	@echo Linking target: $(OUTPUT_NAME).elf
	$(NO_ECHO)$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS) -o $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).elf
	$(NO_ECHO)$(MAKE) -f $(MAKEFILE_NAME) -C $(MAKEFILE_DIR) -e finalize

	@echo "*****************************************************"
	@echo "build project: $(OUTPUT_NAME)"
	@echo "build type:    $(BUILD_TYPE)"
	@echo "build with:    $(TOOLCHAIN_BASE)"
	@echo "build target:  $(TARGET_BOARD)"
	@echo "build options  --"
	@echo "               USE_DFU             $(USE_DFU)"
	@echo "build products --"
	@echo "               $(OUTPUT_NAME).elf"
	@echo "               $(OUTPUT_NAME).hex"
	@echo "*****************************************************"

debug : all

release : all

# Create build directories
$(BUILD_DIRECTORIES):
	echo $(MAKEFILE_NAME)
	$(MK) $@

# Create objects from C SRC files
$(OBJECT_DIRECTORY)/%.o: %.c
	@echo Compiling file: $(notdir $<)
	$(NO_ECHO)$(CC) $(CFLAGS) $(INC_PATHS) \
	-c $< -o $@ > $(OUTPUT_BINARY_DIRECTORY)/$*.lst

# Create objects from C++ SRC files
$(OBJECT_DIRECTORY)/%.o: %.cpp
	@echo Compiling file: $(notdir $<)
	$(NO_ECHO)$(CXX) $(CXXFLAGS) $(CXX_INC_PATHS) \
	-c $< -o $@ > $(OUTPUT_BINARY_DIRECTORY)/$*.lst

# Assemble files
$(OBJECT_DIRECTORY)/%.o: %.s
	@echo Compiling file: $(notdir $<)
	$(NO_ECHO)$(CC) $(ASMFLAGS) $(INC_PATHS) -c -o $@ $<

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

finalize: genbin genhex echosize

genbin:
	@echo Preparing: $(OUTPUT_NAME).bin
	$(NO_ECHO)$(OBJCOPY) -O binary $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).elf $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).bin

# Create binary .hex file from the .elf file
genhex:
	@echo Preparing: $(OUTPUT_NAME).hex
	$(NO_ECHO)$(OBJCOPY) -O ihex $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).elf $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).hex

echosize:
	-@echo ""
	$(NO_ECHO)$(SIZE) $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).elf
	-@echo ""

.PHONY: install
install: all
	@echo Installing: $(OUTPUT_NAME).hex
	$(NO_ECHO)$(RFD_LOADER) $(SERIAL_PORT) $(OUTPUT_BINARY_DIRECTORY)/$(OUTPUT_NAME).hex

clean:
	$(RM) $(BUILD_DIRECTORIES)

cleanobj:
	$(RM) $(BUILD_DIRECTORIES)/*.o

.PHONY: program
program: all install
	./pyaci/configure_sensor.py $(SENSOR_CONFIGURATION_OPTIONS) -d $(SERIAL_PORT) $(SENSOR_ID)
