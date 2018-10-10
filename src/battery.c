#include "battery.h"
#include "assert.h"

#include "app_timer.h"
#include "app_util_platform.h"
#include "bsp.h"
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_drv_ppi.h"
#include "nrf_drv_saadc.h"
#include "nrf_drv_timer.h"
#include "power_manage.h"
#include "assert.h"
#include "softdevice_handler.h"
#include <stdint.h>
#include <string.h>

void saadc_event_handler(nrf_drv_saadc_evt_t const *p_event) {
  log("saadc event");
}

float get_battery_voltage() {
  float voltage;
  // Initialize ADC
  nrf_drv_saadc_config_t saadc_config = NRF_DRV_SAADC_DEFAULT_CONFIG;
  saadc_config.resolution = BATTERY_SENSE_ADC_RESOLUTION;
  ret_code_t err_code = nrf_drv_saadc_init(&saadc_config, saadc_event_handler);
  APP_ERROR_CHECK(err_code);

  // Initialize ADC channel
  nrf_saadc_channel_config_t config =
      NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(BATTERY_SENSE_PIN);
  config.gain = BATTERY_SENSE_ADC_GAIN;
  err_code = nrf_drv_saadc_channel_init(0, &config);
  APP_ERROR_CHECK(err_code);

  // Get an ADC reading
  nrf_saadc_value_t val;
  err_code = nrf_drv_saadc_sample_convert(0, &val);
  APP_ERROR_CHECK(err_code);

  // Uninitialize channel (Not strictly necessary; the call to
  // nrf_drv_saadc_uninit below should handle this)
  err_code = nrf_drv_saadc_channel_uninit(0);
  APP_ERROR_CHECK(err_code);

  // Uninitialize ADC
  nrf_drv_saadc_uninit();

  logf("raw battery adc = %d", val);

  voltage = val / BATTERY_SENSE_ADC_SCALE / BATTERY_SENSE_EXTERNAL_SCALE;

  logf("battery voltage * 100 = %d", (uint16_t)(voltage * 100));

  return voltage;
}
