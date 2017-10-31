#include "pstorage.h"
#include "config.h"
#include "nrf_error.h"
#include <string.h>
#include "leds.h"

// Should be bigger than app_config_t
#define APP_CONFIG_BLOCK_SIZE 32
#define APP_CONFIG_BLOCK 0
#define APP_CONFIG_OFFSET 0

#define APP_CONFIG_STORAGE_SIZE ((sizeof(app_config_t)/32 + 1) * 32)

static pstorage_handle_t m_storage_handle;
static app_config_t m_config;
static bool m_loaded = false;

static void m_storage_callback(pstorage_handle_t *p_handle, uint8_t op_code, uint32_t result, uint8_t *p_data, uint32_t data_len) {
  static pstorage_handle_t block;
  switch (op_code) {
    case PSTORAGE_STORE_OP_CODE:
      if (result == NRF_SUCCESS) {
        TOGGLE_LED(LED_BLUE);
      }
      break;
    case PSTORAGE_CLEAR_OP_CODE:
      // After clearing, we write the config
      if (pstorage_block_identifier_get(&m_storage_handle, APP_CONFIG_BLOCK, &block) == NRF_SUCCESS) {
        pstorage_store(&block, (uint8_t*)&m_config, APP_CONFIG_STORAGE_SIZE, APP_CONFIG_OFFSET);
      }
      break;
  }
}

bool config_init() {
  if (pstorage_init() != NRF_SUCCESS) {
    return false;
  }

  pstorage_module_param_t params = {
    m_storage_callback,
    APP_CONFIG_BLOCK_SIZE,  // Block size
    1                       // Block count
  };

  if (pstorage_register(&params, &m_storage_handle) != NRF_SUCCESS) {
    return false;
  }

  return true;
}

static void init_config_to_defaults() {
  m_config.sensor_id = 0;
  m_config.serial_enabled = 1;
  m_config.mesh_channel = DEFAULT_MESH_CHANNEL;
  m_config.sleep_enabled = 1;
}

static bool ensure_config_loaded() {
  if (!m_loaded) {
    static pstorage_handle_t block;
    if (pstorage_block_identifier_get(&m_storage_handle, APP_CONFIG_BLOCK, &block) != NRF_SUCCESS) {
      return false;
    }

    if (pstorage_load((uint8_t*)&m_config, &block, sizeof(app_config_t), APP_CONFIG_OFFSET) != NRF_SUCCESS) {
      return false;
    }
    if (m_config.sensor_id == 0xff) {
      // Flash has not been written yet.  Initialize config to default values
      init_config_to_defaults();
    }
    m_loaded = true;
  }
  return true;
}

bool get_config(app_config_t *config) {
  if (!ensure_config_loaded()) {
    return false;
  }
  memcpy(config, &m_config, sizeof(app_config_t));
  return true;
}

uint32_t set_config(app_config_t *config) {
  memcpy(&m_config, config, sizeof(app_config_t));
  static pstorage_handle_t block;
  uint32_t error_code;
  error_code = pstorage_block_identifier_get(&m_storage_handle, APP_CONFIG_BLOCK, &block);
  if (error_code != NRF_SUCCESS) {
    return error_code;
  }
  error_code = pstorage_clear(&block, APP_CONFIG_STORAGE_SIZE);
  return error_code;
}

uint8_t get_sensor_id() {
  if (!ensure_config_loaded()) {
    return 0;
  }
  return m_config.sensor_id;
}
