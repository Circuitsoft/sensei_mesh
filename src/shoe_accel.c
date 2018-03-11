#ifdef ACCEL_ADXL337

#include "shoe_accel.h"
#include "gpio_pins.h"
#include "nrf_gpio.h"
#include "wiring_analog.h"

#define ACCEL_POWER_PIN 23
#define X_PIN 4
#define Z_PIN 6

void shoe_accel_init() {
  nrf_gpio_cfg_output(ACCEL_POWER_PIN);
  nrf_gpio_cfg_input(X_PIN, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(Z_PIN, NRF_GPIO_PIN_NOPULL);
}

void enable_shoe_accel() {
  SET_PIN(ACCEL_POWER_PIN);
}

void disable_shoe_accel() {
  CLEAR_PIN(ACCEL_POWER_PIN);
}

void read_shoe_accel(int8_t *x, int8_t *y, int8_t *z) {
  *x = (int8_t)(analogRead(X_PIN) - 512 / 4);
  *y = 0; // Y is not connected currently
  *z = (int8_t)(analogRead(Z_PIN) - 512 / 4);
}

#endif