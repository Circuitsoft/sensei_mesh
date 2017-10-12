#include "shoe_accel.h"
#include "gpio_pins.h"
#include "nrf_gpio.h"

#define ACCEL_POWER_PIN 23
#define X_PIN 4
#define Z_PIN 6

void shoe_accel_init() {
  nrf_gpio_cfg_output(ACCEL_POWER_PIN);
  nrf_gpio_cfg_input(X_PIN, NRF_GPIO_PIN_NOPULL);
  nrf_gpio_cfg_input(Z_PIN, NRF_GPIO_PIN_NOPULL);
}

static const int32_t adc_config_base = // 8-bit is 0
                    (ADC_CONFIG_INPSEL_AnalogInputOneThirdPrescaling << ADC_CONFIG_INPSEL_Pos) |
                    (ADC_CONFIG_EXTREFSEL_AnalogReference0 << ADC_CONFIG_EXTREFSEL_Pos);

static const int32_t adc_config_batt = // 8-bit is 0
                    (ADC_CONFIG_INPSEL_SupplyTwoThirdsPrescaling << ADC_CONFIG_INPSEL_Pos);
                    // Bandgap Reference is 0
                    // No external reference is 0

void enable_shoe_accel() {
  SET_PIN(ACCEL_POWER_PIN);
}

static int8_t readPin(int pin)
{
    uint8_t retval;
    NRF_ADC->CONFIG = adc_config_base |
                      (1 << (pin + 1 + ADC_CONFIG_PSEL_Pos));
    NRF_ADC->INTENCLR = 0xffffffff;
    NRF_ADC->ENABLE = ADC_ENABLE_ENABLE_Enabled << ADC_ENABLE_ENABLE_Pos;
    NRF_ADC->EVENTS_END = 0;
    NRF_ADC->TASKS_START = 1;
    while (!NRF_ADC->EVENTS_END);
    retval = NRF_ADC->RESULT;
    NRF_ADC->ENABLE = ADC_ENABLE_ENABLE_Disabled << ADC_ENABLE_ENABLE_Pos;
    NRF_ADC->TASKS_STOP = 1;
    NRF_ADC->CONFIG = adc_config_batt;
    return retval % -128;
}

void disable_shoe_accel() {
  CLEAR_PIN(ACCEL_POWER_PIN);
}

void read_shoe_accel(int8_t *x, int8_t *y, int8_t *z) {
  *x = readPin(X_PIN);
  *y = 0; // Y is not connected currently
  *z = readPin(Z_PIN);
}
