#include "jostle_detect.h"

#ifdef BMX055

#include "app_error.h"
#include "assert.h"
#include "bsp.h"
#include "i2c.h"
#include "nrf_drv_gpiote.h"
#include "nrf_delay.h"
#include "rtc.h"

#define BMX055_ADDR_ACCEL (0x18)
#define BMX055_ADDR_GYRO (0x68)
#define BMX055_ADDR_MAG (0x10)
// Accelerometer registers
#define BMX055_ACC_WHOAMI (0x00) // should return 0xFA
//#define BMX055_ACC_Reserved    (0x01)
#define BMX055_ACC_D_X_LSB (0x02)
#define BMX055_ACC_D_X_MSB (0x03)
#define BMX055_ACC_D_Y_LSB (0x04)
#define BMX055_ACC_D_Y_MSB (0x05)
#define BMX055_ACC_D_Z_LSB (0x06)
#define BMX055_ACC_D_Z_MSB (0x07)
#define BMX055_ACC_D_TEMP (0x08)
#define BMX055_ACC_INT_STATUS_0 (0x09)
#define BMX055_ACC_INT_STATUS_1 (0x0A)
#define BMX055_ACC_INT_STATUS_2 (0x0B)
#define BMX055_ACC_INT_STATUS_3 (0x0C)
//#define BMX055_ACC_Reserved    (0x0D)
#define BMX055_ACC_FIFO_STATUS (0x0E)
#define BMX055_ACC_PMU_RANGE (0x0F)
#define BMX055_ACC_PMU_BW (0x10)
#define BMX055_ACC_PMU_LPW (0x11)
#define BMX055_ACC_PMU_LOW_POWER (0x12)
#define BMX055_ACC_D_HBW (0x13)
#define BMX055_ACC_BGW_SOFTRESET (0x14)
//#define BMX055_ACC_Reserved    (0x15)
#define BMX055_ACC_INT_EN_0 (0x16)
#define BMX055_ACC_INT_EN_1 (0x17)
#define BMX055_ACC_INT_EN_2 (0x18)
#define BMX055_ACC_INT_MAP_0 (0x19)
#define BMX055_ACC_INT_MAP_1 (0x1A)
#define BMX055_ACC_INT_MAP_2 (0x1B)
//#define BMX055_ACC_Reserved    (0x1C)
//#define BMX055_ACC_Reserved    (0x1D)
#define BMX055_ACC_INT_SRC (0x1E)
//#define BMX055_ACC_Reserved    (0x1F)
#define BMX055_ACC_INT_OUT_CTRL (0x20)
#define BMX055_ACC_INT_RST_LATCH (0x21)
#define BMX055_ACC_INT_0 (0x22)
#define BMX055_ACC_INT_1 (0x23)
#define BMX055_ACC_INT_2 (0x24)
#define BMX055_ACC_INT_3 (0x25)
#define BMX055_ACC_INT_4 (0x26)
#define BMX055_ACC_INT_5 (0x27)
#define BMX055_ACC_INT_6 (0x28)
#define BMX055_ACC_INT_7 (0x29)
#define BMX055_ACC_INT_8 (0x2A)
#define BMX055_ACC_INT_9 (0x2B)
#define BMX055_ACC_INT_A (0x2C)
#define BMX055_ACC_INT_B (0x2D)
#define BMX055_ACC_INT_C (0x2E)
#define BMX055_ACC_INT_D (0x2F)
#define BMX055_ACC_FIFO_CONFIG_0 (0x30)
//#define BMX055_ACC_Reserved    (0x31)
#define BMX055_ACC_PMU_SELF_TEST (0x32)
#define BMX055_ACC_TRIM_NVM_CTRL (0x33)
#define BMX055_ACC_BGW_SPI3_WDT (0x34)
//#define BMX055_ACC_Reserved    (0x35)
#define BMX055_ACC_OFC_CTRL (0x36)
#define BMX055_ACC_OFC_SETTING (0x37)
#define BMX055_ACC_OFC_OFFSET_X (0x38)
#define BMX055_ACC_OFC_OFFSET_Y (0x39)
#define BMX055_ACC_OFC_OFFSET_Z (0x3A)
#define BMX055_ACC_TRIM_GPO (0x3B)
#define BMX055_ACC_TRIM_GP1 (0x3C)
//#define BMX055_ACC_Reserved    (0x3D)
#define BMX055_ACC_FIFO_CONFIG_1 (0x3E)
#define BMX055_ACC_FIFO_DATA (0x3F)
// BMX055 Gyroscope Registers
#define BMX055_GYRO_WHOAMI (0x00) // should return 0x0F
//#define BMX055_GYRO_Reserved       (0x01)
#define BMX055_GYRO_RATE_X_LSB (0x02)
#define BMX055_GYRO_RATE_X_MSB (0x03)
#define BMX055_GYRO_RATE_Y_LSB (0x04)
#define BMX055_GYRO_RATE_Y_MSB (0x05)
#define BMX055_GYRO_RATE_Z_LSB (0x06)
#define BMX055_GYRO_RATE_Z_MSB (0x07)
//#define BMX055_GYRO_Reserved       (0x08)
#define BMX055_GYRO_INT_STATUS_0 (0x09)
#define BMX055_GYRO_INT_STATUS_1 (0x0A)
#define BMX055_GYRO_INT_STATUS_2 (0x0B)
#define BMX055_GYRO_INT_STATUS_3 (0x0C)
//#define BMX055_GYRO_Reserved    (0x0D)
#define BMX055_GYRO_FIFO_STATUS (0x0E)
#define BMX055_GYRO_RANGE (0x0F)
#define BMX055_GYRO_BW (0x10)
#define BMX055_GYRO_LPM1 (0x11)
#define BMX055_GYRO_LPM2 (0x12)
#define BMX055_GYRO_RATE_HBW (0x13)
#define BMX055_GYRO_BGW_SOFTRESET (0x14)
#define BMX055_GYRO_INT_EN_0 (0x15)
#define BMX055_GYRO_INT_EN_1 (0x16)
#define BMX055_GYRO_INT_MAP_0 (0x17)
#define BMX055_GYRO_INT_MAP_1 (0x18)
#define BMX055_GYRO_INT_MAP_2 (0x19)
#define BMX055_GYRO_INT_SRC_1 (0x1A)
#define BMX055_GYRO_INT_SRC_2 (0x1B)
#define BMX055_GYRO_INT_SRC_3 (0x1C)
//#define BMX055_GYRO_Reserved    (0x1D)
#define BMX055_GYRO_FIFO_EN (0x1E)
//#define BMX055_GYRO_Reserved    (0x1F)
//#define BMX055_GYRO_Reserved    (0x20)
#define BMX055_GYRO_INT_RST_LATCH (0x21)
#define BMX055_GYRO_HIGH_TH_X (0x22)
#define BMX055_GYRO_HIGH_DUR_X (0x23)
#define BMX055_GYRO_HIGH_TH_Y (0x24)
#define BMX055_GYRO_HIGH_DUR_Y (0x25)
#define BMX055_GYRO_HIGH_TH_Z (0x26)
#define BMX055_GYRO_HIGH_DUR_Z (0x27)
//#define BMX055_GYRO_Reserved    (0x28)
//#define BMX055_GYRO_Reserved    (0x29)
//#define BMX055_GYRO_Reserved    (0x2A)
#define BMX055_GYRO_SOC (0x31)
#define BMX055_GYRO_A_FOC (0x32)
#define BMX055_GYRO_TRIM_NVM_CTRL (0x33)
#define BMX055_GYRO_BGW_SPI3_WDT (0x34)
//#define BMX055_GYRO_Reserved    (0x35)
#define BMX055_GYRO_OFC1 (0x36)
#define BMX055_GYRO_OFC2 (0x37)
#define BMX055_GYRO_OFC3 (0x38)
#define BMX055_GYRO_OFC4 (0x39)
#define BMX055_GYRO_TRIM_GP0 (0x3A)
#define BMX055_GYRO_TRIM_GP1 (0x3B)
#define BMX055_GYRO_BIST (0x3C)
#define BMX055_GYRO_FIFO_CONFIG_0 (0x3D)
#define BMX055_GYRO_FIFO_CONFIG_1 (0x3E)
// BMX055 magnetometer registers
#define BMX055_MAG_WHOAMI (0x40) // should return 0x32
#define BMX055_MAG_Reserved (0x41)
#define BMX055_MAG_XOUT_LSB (0x42)
#define BMX055_MAG_XOUT_MSB (0x43)
#define BMX055_MAG_YOUT_LSB (0x44)
#define BMX055_MAG_YOUT_MSB (0x45)
#define BMX055_MAG_ZOUT_LSB (0x46)
#define BMX055_MAG_ZOUT_MSB (0x47)
#define BMX055_MAG_ROUT_LSB (0x48)
#define BMX055_MAG_ROUT_MSB (0x49)
#define BMX055_MAG_INT_STATUS (0x4A)
#define BMX055_MAG_PWR_CNTL1 (0x4B)
#define BMX055_MAG_PWR_CNTL2 (0x4C)
#define BMX055_MAG_INT_EN_1 (0x4D)
#define BMX055_MAG_INT_EN_2 (0x4E)
#define BMX055_MAG_LOW_THS (0x4F)
#define BMX055_MAG_HIGH_THS (0x50)
#define BMX055_MAG_REP_XY (0x51)
#define BMX055_MAG_REP_Z (0x52)
/* Trim Extended Registers */
#define BMM050_DIG_X1 (0x5D) // needed for magnetic field calculation
#define BMM050_DIG_Y1 (0x5E)
#define BMM050_DIG_Z4_LSB (0x62)
#define BMM050_DIG_Z4_MSB (0x63)
#define BMM050_DIG_X2 (0x64)
#define BMM050_DIG_Y2 (0x65)
#define BMM050_DIG_Z2_LSB (0x68)
#define BMM050_DIG_Z2_MSB (0x69)
#define BMM050_DIG_Z1_LSB (0x6A)
#define BMM050_DIG_Z1_MSB (0x6B)
#define BMM050_DIG_XYZ1_LSB (0x6C)
#define BMM050_DIG_XYZ1_MSB (0x6D)
#define BMM050_DIG_Z3_LSB (0x6E)
#define BMM050_DIG_Z3_MSB (0x6F)
#define BMM050_DIG_XY2 (0x70)
#define BMM050_DIG_XY1 (0x71)

static bool jostle_detected;


static bool write_acc_register(uint8_t reg, uint8_t value) {
  return i2c_write_reg(BMX055_ADDR_ACCEL, reg, value);
}

static uint8_t read_acc_register(uint8_t reg) {
  return i2c_read_reg(BMX055_ADDR_ACCEL, reg);
}

volatile uint32_t last_jostle;

// Motion interrupt pin handler
void motion_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
  log("motion handler int");

  last_jostle = rtc_value();
  log("jostle detected");

  jostle_detected = true;
}

void jostle_detect_init() {
  log("jostle_detect_init");
  i2c_init();

  ret_code_t err_code;

  // Configure bmx055
  // reset
  log("reset BMX055");
  write_acc_register(BMX055_ACC_BGW_SOFTRESET, 0xB6);
  nrf_delay_us(2048); // Wait for initialization

  uint8_t data = read_acc_register(BMX055_ACC_WHOAMI);

  if (data) {
    logf("BMX055_ACC addr = 0x%x", data);
  } else {
    log("Could not read accelerometer");
  }

  // enable 4G range (datasheet sec. 5.2.1)
  write_acc_register(BMX055_ACC_PMU_RANGE, 0x05);
  // enable 7.81Hz BW (datasheet sec. 5.2.1)
  write_acc_register(BMX055_ACC_PMU_BW, 8);
  // Low power mode
  write_acc_register(BMX055_ACC_PMU_LPW, (1<<6) | // lowpower_en
                                         (10<<1)); // sleep_dur = 10ms
  write_acc_register(BMX055_ACC_PMU_LOW_POWER, (1<<6)); // lowpower_mode = 2

#ifdef JOSTLE_WAKEUP
  // Setup motion detection
  write_acc_register(BMX055_ACC_INT_5, 3); // 2 consecutive samples for slope detection, no slow-motion detection
  write_acc_register(BMX055_ACC_INT_6, JOSTLE_SLOPE_THRESH); // Defined per-device
  write_acc_register(BMX055_ACC_INT_EN_0, 7); // Enable slope on X, Y, and Z
#endif // JOSTLE_WAKEUP

  // Save power after configuring accelerometer
  i2c_shutdown();

  // Set up Interrupt Line
  log("Configure BMX055 Interrupt");
  err_code = nrf_drv_gpiote_init();
  APP_ERROR_CHECK(err_code);

  nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_HITOLO(false);
  in_config.pull = NRF_GPIO_PIN_PULLUP;

  err_code = nrf_drv_gpiote_in_init(IMU_INT1_GPIO, &in_config, motion_handler);
  APP_ERROR_CHECK(err_code);

  nrf_drv_gpiote_in_event_enable(IMU_INT1_GPIO, true);
}

static inline float fabs(float val)
{
  return (val < 0) ? 0 - val : val;
}

void jostle_detect_check()
{
  i2c_init();
  int16_t x, y, z;
  float fx, fy, fz, facc;
  static float running_acc, veryslow_acc;
  static bool need_init = true;

  x  = ((int16_t)read_acc_register(BMX055_ACC_D_X_LSB)) >> 4;
  x |= ((int16_t)read_acc_register(BMX055_ACC_D_X_MSB)) << 4;
  y  = ((int16_t)read_acc_register(BMX055_ACC_D_Y_LSB)) >> 4;
  y |= ((int16_t)read_acc_register(BMX055_ACC_D_Y_MSB)) << 4;
  z  = ((int16_t)read_acc_register(BMX055_ACC_D_Z_LSB)) >> 4;
  z |= ((int16_t)read_acc_register(BMX055_ACC_D_Z_MSB)) << 4;
  i2c_shutdown();

  fx = ((float)x) / 4096.0f;
  fy = ((float)y) / 4096.0f;
  fz = ((float)z) / 4096.0f;
  facc = fx*fx + fy*fy + fz*fz;
  asm("vsqrt.f32 %0, %0" : "=w" (facc) : "w" (facc) );

  if (need_init)
  {
    veryslow_acc = running_acc = facc;
    need_init = false;
  }

  running_acc *= 0.7;
  running_acc += facc * 0.3;

  veryslow_acc *= 0.7;
  veryslow_acc += running_acc * 0.3;

  if (fabs(veryslow_acc - running_acc) > JOSTLE_AVERAGE_THRESH)
    motion_handler(0, 0); // unused parameters
}

#ifdef JOSTLE_WAKEUP
void jostle_detect_enable()
{
  i2c_init();
  write_acc_register(BMX055_ACC_INT_MAP_0, 4); // Enable slope on INT1
  i2c_shutdown();
}

void jostle_detect_disable()
{
  i2c_init();
  write_acc_register(BMX055_ACC_INT_MAP_0, 0); // Disable slope on INT1
  i2c_shutdown();
}
#endif // JOSTLE_WAKEUP

bool jostle_detect_get_flag() { return jostle_detected; }

void jostle_detect_clear_flag() { jostle_detected = false; }

#endif // BMX055
