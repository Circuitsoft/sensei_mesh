

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
#include "transport_control.h"
#include "serial_handler.h"
#include "scheduler.h"
#include "heartbeat.h"
#include "handles.h"
#include "bsp.h"
#include "mesh_control.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>


static nrf_clock_lf_cfg_t clock_lf_cfg = NRF_CLOCK_LFCLKSRC;
#define MESH_ACCESS_ADDR        (0xA555410C)
#define MESH_INTERVAL_MIN_MS    (100)
#define MESH_CLOCK_SRC          (clock_lf_cfg)

/** @brief General error handler. */
static inline void error_loop(void)
{
  TOGGLE_LED(LED_RED);
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
void sd_assert_handler(uint32_t id, uint32_t pc, uint32_t info)
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

  switch (p_evt->type)
  {
    case RBC_MESH_EVENT_TYPE_CONFLICTING_VAL:
    case RBC_MESH_EVENT_TYPE_NEW_VAL:
    case RBC_MESH_EVENT_TYPE_UPDATE_VAL:
      if (p_evt->params.rx.value_handle == MESH_CONTROL_HANDLE && p_evt->params.rx.data_len == sizeof(mesh_control_t)) {
        mesh_control_update_config((mesh_control_t*)p_evt->params.rx.p_data);
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

static void packet_peek_cb(rbc_mesh_packet_peek_params_t *params) {
  if (params->packet_type == BLE_PACKET_TYPE_ADV_NONCONN_IND &&
      params->p_payload[1] == HEARTBEAT_ADV_DATA_TYPE) {
    received_heartbeat((heartbeat_ad_t*)&params->p_payload[2], params->rssi);
  }
}

int main(void)
{

  bsp_init(BSP_INIT_BUTTONS & BSP_INIT_LED, 0, 0);

  mesh_control_init();

#if LEDS_NUMBER > 0
  nrf_gpio_cfg_output(LED_START + LED_GREEN);
  nrf_gpio_cfg_output(LED_START + LED_RED);
  nrf_gpio_cfg_output(LED_START + LED_BLUE);
#endif

  /* Enable Softdevice (including sd_ble before framework */
  SOFTDEVICE_HANDLER_INIT(&MESH_CLOCK_SRC, NULL);
  softdevice_ble_evt_handler_set(rbc_mesh_ble_evt_handler);

  //clock_initialization();
  //sd_softdevice_disable();

  // Register with the SoftDevice handler module for system events.
  softdevice_sys_evt_handler_set(sys_evt_dispatch);

  // Debug pins
  // nrf_gpio_cfg_output(5);
  // nrf_gpio_cfg_output(6);

  // Disable simblee's proximityMode
#ifdef SIMBLEE
  nrf_gpio_cfg_output(31);
  CLEAR_PIN(31);
#endif

  // if (NRF_CLOCK->LFCLKSRC == (CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos)) {
  //   TOGGLE_LED(LED_GREEN);
  // }

  /* Initialize mesh. */
  rbc_mesh_init_params_t init_params;
  init_params.access_addr = MESH_ACCESS_ADDR;
  init_params.interval_min_ms = MESH_INTERVAL_MIN_MS;
  init_params.channel = DEFAULT_MESH_CHANNEL;
  init_params.lfclksrc = MESH_CLOCK_SRC;
  init_params.tx_power = RBC_MESH_TXPOWER_0dBm;

  uint32_t error_code;
  error_code = rbc_mesh_init(init_params);
  APP_ERROR_CHECK(error_code);
  //led_config(LED_GREEN, 1);


  // This has to come after rbc_mesh_init for some reason.  Otherwise we
  // get a HardFault when rbc_mesh_init is called
  app_config_t app_config;
  config_init();
  get_config(&app_config);

  // Setup handler for watching for heartbeat messages
  rbc_mesh_packet_peek_cb_set(packet_peek_cb);

  // Change channel if needed
  // if (app_config.mesh_channel != DEFAULT_MESH_CHANNEL) {
  //   tc_radio_params_set(MESH_ACCESS_ADDR, app_config.mesh_channel);
  // }

  scheduler_init(app_config.sleep_enabled); // Initializes, but does not start, clock
  heartbeat_init(DEFAULT_MESH_CHANNEL); // Inits structures for sending heartbeat

  /* Initialize mesh ACI */
  mesh_aci_init();
  mesh_aci_app_cmd_handler_set(app_cmd_handler);

  // Stop serial if serial is disabled
  if (!app_config.serial_enabled) {
    serial_handler_stop();
  }

  /* Enable our handle */
  if (app_config.sensor_id > 0) {
    sensor_init();
  }

  error_code = rbc_mesh_value_enable(MESH_CONTROL_HANDLE);
  APP_ERROR_CHECK(error_code);

  // error_code = rbc_mesh_value_enable(TEST_LED_HANDLE);
  // APP_ERROR_CHECK(error_code);

  // Start clock
  start_clock(0);

   //rbc_mesh_stop();
  //rbc_mesh_start();
  //sd_app_evt_wait();
  // while(1) {
  //   __WFE();
  //   __SEV();
  //   __WFE();
  // }

  rbc_mesh_event_t evt;
  while (true) {
    if (rbc_mesh_event_get(&evt) == NRF_SUCCESS) {
      rbc_mesh_event_handler(&evt);
      rbc_mesh_event_release(&evt);
    }
    sd_app_evt_wait();
  }
}
