
#include "jostle_detect.h"

#ifdef MMA8541

#include "app_error.h"
#include "assert.h"
#include "i2c.h"
#include "nrf_drv_gpiote.h"
#include "rtc.h"

#define MMA8451_DEFAULT_ADDRESS (0x1C) // if A is GND, it's 0x1C

#define MMA8451_REG_OUT_X_MSB 0x01
#define MMA8451_REG_SYSMOD 0x0B
#define MMA8451_REG_WHOAMI 0x0D
#define MMA8451_REG_XYZ_DATA_CFG 0x0E
#define MMA8451_REG_PL_STATUS 0x10
#define MMA8451_REG_PL_CFG 0x11
#define MMA8451_REG_FF_MT_CFG 0x15
#define MMA8451_REG_FF_MT_SRC 0x16
#define MMA8451_REG_FF_MT_THS 0x17
#define MMA8451_REG_FF_MT_COUNT 0x18
#define MMA8451_REG_CTRL_REG1 0x2A
#define MMA8451_REG_CTRL_REG2 0x2B
#define MMA8451_REG_CTRL_REG4 0x2D
#define MMA8451_REG_CTRL_REG5 0x2E

#define INT_EN_FF_MT (1 << 2)
#define INT_EN_PULSE (1 << 3)

#define INT1_GPIO_PIN 24
#define INT2_GPIO_PIN 22

static bool jostle_detected;

typedef enum {
  MMA8451_RANGE_8_G = 0b10, // +/- 8g
  MMA8451_RANGE_4_G = 0b01, // +/- 4g
  MMA8451_RANGE_2_G = 0b00  // +/- 2g (default value)
} mma8451_range_t;

#define MMA8451_HIGH_PASS_FILTER 0b10000

static bool write_register(uint8_t reg, uint8_t value) {
  return i2c_write_reg(MMA8451_DEFAULT_ADDRESS, reg, value);
}

static uint8_t read_register(uint8_t reg) {
  return i2c_read_reg(MMA8451_DEFAULT_ADDRESS, reg);
}

volatile uint32_t last_jostle;

// Motion interrupt pin handler
void motion_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
  last_jostle = rtc_value();
  log("jostle detected");

  i2c_init();
  // Read interrupt source register (clears interrupt)
  read_register(MMA8451_REG_FF_MT_SRC);
  i2c_shutdown();

  jostle_detected = true;
}

void jostle_detect_init() {
  i2c_init();

  ret_code_t err_code;

  err_code = nrf_drv_gpiote_init();
  APP_ERROR_CHECK(err_code);

  nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(false);
  in_config.pull = NRF_GPIO_PIN_PULLUP;

  err_code = nrf_drv_gpiote_in_init(INT1_GPIO_PIN, &in_config, motion_handler);
  APP_ERROR_CHECK(err_code);

  nrf_drv_gpiote_in_event_enable(INT1_GPIO_PIN, true);

  // Configure mma8451
  write_register(MMA8451_REG_CTRL_REG2, 0x40); // reset

  uint8_t data = 0;

  while (!i2c_read_data(MMA8451_DEFAULT_ADDRESS, MMA8451_REG_CTRL_REG2, &data,
                        1) ||
         data & 0x40)
    ;

  // enable 4G range
  write_register(MMA8451_REG_XYZ_DATA_CFG, MMA8451_RANGE_4_G);

  // Low power mode
  write_register(MMA8451_REG_CTRL_REG2, 0b00000011);

  // Setup motion detection
  write_register(MMA8451_REG_FF_MT_CFG, 0b11111000); // Enable motion, and x,y,z
  write_register(MMA8451_REG_FF_MT_THS, 17);  // Threshold: 17 * 0.063g = 1.071g
  write_register(MMA8451_REG_FF_MT_COUNT, 1); // debounce counter: 20ms @ 50hz

  // Setup interrupts
  write_register(MMA8451_REG_CTRL_REG4,
                 INT_EN_FF_MT |
                     INT_EN_PULSE); // Enable Freefall/Motion int, and pulse
  write_register(MMA8451_REG_CTRL_REG5, 0b00000100); // Route motion to INT1

  // Turn on orientation config
  // write_register(MMA8451_REG_PL_CFG, 0x40);

  // ASLP_RATE = 50hz
  // ODR = 50hz
  // LNOISE = 1
  // F_READ = normal
  // ACTIVE = active
  write_register(MMA8451_REG_CTRL_REG1, 0b00100101);

  // Save power after configuring accelerometer
  i2c_shutdown();
}

bool jostle_detect_get_flag() { return jostle_detected; }

void jostle_detect_clear_flag() { jostle_detected = false; }

#endif // MMA8541
