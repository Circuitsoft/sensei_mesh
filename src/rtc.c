#include "rtc.h"

// Determines the RTC interrupt frequency
const uint8_t RTC_CC_VALUE = 24;

// Use RTC2 (Note: RTC0 is used by the softdevice)
const nrf_drv_rtc_t rtc = NRF_DRV_RTC_INSTANCE(2);

volatile uint32_t rtc_count = 0;

uint32_t rtc_value(void) { return rtc_count; }

// Assumes prescaler of 4095
// TODO: Add prescaler value assertion
uint32_t rtc_to_walltime(uint32_t rtc_value) { return rtc_value * 3; }

void rtc_handler(nrf_drv_rtc_int_type_t int_type) {
  uint32_t err_code;

  if (int_type == NRF_DRV_RTC_INT_COMPARE0) {
    rtc_count++;

    err_code = nrf_drv_rtc_cc_set(&rtc, 0, RTC_CC_VALUE,
                                  true); // Set RTC compare value. This needs to
                                         // be done every time as the
                                         // nrf_drv_rtc clears the compare
                                         // register on every compare match
    APP_ERROR_CHECK(err_code);

    // Clear the RTC counter to start count from zero
    nrf_drv_rtc_counter_clear(&rtc);
  }
}

void rtc_init(void) {
  uint32_t err_code;

  // Initialize RTC instance
  nrf_drv_rtc_config_t rtc_config;
  rtc_config.prescaler = 4095; // 125ms counter resolution

  // Initialize the RTC with callback
  // function rtc_handler. The
  // rtc_handler must be implemented
  // in this applicaiton. Passing NULL
  // here for RTC configuration means
  // that configuration will be taken
  // from the sdk_config.h file.
  err_code = nrf_drv_rtc_init(&rtc, &rtc_config, rtc_handler);
  APP_ERROR_CHECK(err_code);

  // Set RTC compare value to trigger interrupt
  err_code = nrf_drv_rtc_cc_set(&rtc, 0, RTC_CC_VALUE, true);
  APP_ERROR_CHECK(err_code);

  // Power on RTC instance
  nrf_drv_rtc_enable(&rtc); // Enable RTC
}
