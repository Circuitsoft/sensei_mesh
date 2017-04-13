

#include "rbc_mesh.h"
#include "mesh_aci.h"

#include "softdevice_handler.h"
#include "app_error.h"
#include "nrf_gpio.h"
#include "boards.h"
#include "leds.h"
#include "pstorage_platform.h"
#include "app_cmd.h"
#include "config.h"
#include "sensor.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>


#define MESH_ACCESS_ADDR        (0xA555410C)
#define MESH_INTERVAL_MIN_MS    (100)
#define MESH_CHANNEL            (38)
#define MESH_CLOCK_SRC          (NRF_CLOCK_LFCLKSRC_XTAL_75_PPM)


/** @brief General error handler. */
static inline void error_loop(void)
{
  __disable_irq();
  while (true)
  {
    __WFE();
  }
}

/**
* @brief Softdevice crash handler, never returns
*
* @param[in] pc Program counter at which the assert failed
* @param[in] line_num Line where the error check failed
* @param[in] p_file_name File where the error check failed
*/
void sd_assert_handler(uint32_t pc, uint16_t line_num, const uint8_t* p_file_name)
{
  error_loop();
}

/** @brief Hardware fault handler. */
void HardFault_Handler(void)
{
  error_loop();
}

/**
* @brief Mesh framework event handler.
*
* @param[in] p_evt Mesh event propagated from framework.
*/
static void rbc_mesh_event_handler(rbc_mesh_event_t* p_evt)
{
  toggle_led(LED_GREEN);

  switch (p_evt->type)
  {
    case RBC_MESH_EVENT_TYPE_CONFLICTING_VAL:
    case RBC_MESH_EVENT_TYPE_NEW_VAL:
    case RBC_MESH_EVENT_TYPE_UPDATE_VAL:
      if (p_evt->params.rx.value_handle == 1) {
        led_config(LED_BLUE, p_evt->params.rx.p_data[0]);
      }
      break;
    case RBC_MESH_EVENT_TYPE_TX:
    case RBC_MESH_EVENT_TYPE_INITIALIZED:
    case RBC_MESH_EVENT_TYPE_DFU_NEW_FW_AVAILABLE:
    case RBC_MESH_EVENT_TYPE_DFU_RELAY_REQ:
    case RBC_MESH_EVENT_TYPE_DFU_SOURCE_REQ:
    case RBC_MESH_EVENT_TYPE_DFU_START:
    case RBC_MESH_EVENT_TYPE_DFU_END:
    case RBC_MESH_EVENT_TYPE_DFU_BANK_AVAILABLE:
      break;
  }
}

/* dispatch system events to interested modules. */
static void sys_evt_dispatch(uint32_t sys_evt)
{
    pstorage_sys_event_handler(sys_evt);
    //ble_advertising_on_sys_evt(sys_evt);
}


void clock_initialization()
{
    NRF_CLOCK->LFCLKSRC            = (CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos);
    NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_LFCLKSTART    = 1;

    while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0)
    {
        // Do nothing.
    }

    NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
}


int main(void)
{
  //clock_initialization();

  nrf_gpio_cfg_input(BUTTON_1, NRF_GPIO_PIN_PULLDOWN);
  nrf_gpio_cfg_input(BUTTON_2, NRF_GPIO_PIN_PULLDOWN);

  /* Enable Softdevice (including sd_ble before framework */
  SOFTDEVICE_HANDLER_INIT(MESH_CLOCK_SRC, NULL);
  softdevice_ble_evt_handler_set(rbc_mesh_ble_evt_handler);

  // Register with the SoftDevice handler module for system events.
  softdevice_sys_evt_handler_set(sys_evt_dispatch);

  LEDS_CONFIGURE(LEDS_MASK);

  // if (NRF_CLOCK->LFCLKSRC == (CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos)) {
  //   toggle_led(LED_GREEN);
  // }

  /* Initialize mesh. */
  rbc_mesh_init_params_t init_params;
  init_params.access_addr = MESH_ACCESS_ADDR;
  init_params.interval_min_ms = MESH_INTERVAL_MIN_MS;
  init_params.channel = MESH_CHANNEL;
  init_params.lfclksrc = MESH_CLOCK_SRC;
  init_params.tx_power = RBC_MESH_TXPOWER_0dBm;

  uint32_t error_code;
  error_code = rbc_mesh_init(init_params);
  APP_ERROR_CHECK(error_code);

  config_init();

  /* Initialize serial ACI */
#ifdef RBC_MESH_SERIAL
  mesh_aci_init();
  mesh_aci_app_cmd_handler_set(app_cmd_handler);
#endif

  /* Enable handle 1 */
  error_code = rbc_mesh_value_enable(1);
  APP_ERROR_CHECK(error_code);

  rbc_mesh_event_t evt;

  while (true)
  {

    for (uint32_t pin = BUTTON_START; pin <= BUTTON_STOP; ++pin)
    {
      if(nrf_gpio_pin_read(pin) == 1)
      {
        while(nrf_gpio_pin_read(pin) == 1);
        uint8_t mesh_data[1];
        uint32_t led_status = !!((pin - BUTTON_START) & 0x01); /* even buttons are OFF, odd buttons are ON */

        mesh_data[0] = led_status;
        if (rbc_mesh_value_set(1, mesh_data, 1) == NRF_SUCCESS)
        {
          led_config(LED_BLUE, led_status);
        }
      }
    }

    if (rbc_mesh_event_get(&evt) == NRF_SUCCESS)
    {
      rbc_mesh_event_handler(&evt);
      rbc_mesh_event_release(&evt);
    }

    sd_app_evt_wait();
  }
}
