
#include "mesh_control.h"
#include "leds.h"
#include "transport_control.h"
#include "assert.h"

static mesh_control_t m_config;

void mesh_control_init() {
  m_config.wake_interval = DEFAULT_WAKE_INTERVAL;
  m_config.hb_tx_power = RBC_MESH_TXPOWER_Neg4dBm;
  m_config.enable_ble = 0;
}

uint16_t mesh_control_get_wake_interval() { return m_config.wake_interval; }

uint8_t mesh_control_get_hb_tx_power() { return m_config.hb_tx_power; }

void mesh_control_update_config(mesh_control_t *new_config) {
  rbc_mesh_txpower_t allowed_tx_power[] = {
      RBC_MESH_TXPOWER_0dBm, RBC_MESH_TXPOWER_Pos4dBm,
      RBC_MESH_TXPOWER_Neg12dBm, RBC_MESH_TXPOWER_Neg8dBm,
      RBC_MESH_TXPOWER_Neg4dBm};

  // Sanity checks
  if (new_config->wake_interval == 0 || new_config->wake_interval > 1000) {
    logf("Bad config: new_config->wake_interval=%d out of acceptable range "
        "(1-1000)",
        new_config->wake_interval);
    return;
  }

  bool tx_allowed = false;
  for (int i = 0; i < sizeof(allowed_tx_power) / sizeof(rbc_mesh_txpower_t);
       i++) {
    if (allowed_tx_power[i] == new_config->hb_tx_power) {
      tx_allowed = true;
    }
  }
  if (!tx_allowed) {
    return;
  }

  // Ok, looks good!
  m_config = *new_config;
}
