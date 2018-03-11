#ifndef MESH_CONTROL_H
#define MESH_CONTROL_H

#include "toolchain.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Default wake interval is 10 seconds
#define DEFAULT_WAKE_INTERVAL (10)

typedef __packed_armcc struct {
  uint16_t wake_interval;
  uint8_t hb_tx_power;
  uint8_t enable_ble; // Not used yet...
  uint32_t sleep_time;
  uint32_t wake_time;
} __packed_gcc mesh_control_t;

void mesh_control_init();

// Wake interval controls the cycle of waking and sleeping
// current_epoch % wake_interval == 0 indicates the start of a wake period
uint16_t mesh_control_get_wake_interval();

// Heartbeat tx power controls the tx power level of sent heartbeat packets
uint8_t mesh_control_get_hb_tx_power();

// Sleep and Wake time are absolute unix timestamps for when to shutdown
// immediately instead of waking and doing stuff.
int mesh_control_should_sleep(uint32_t timestamp);

void mesh_control_update_config(mesh_control_t *new_config);

#ifdef __cplusplus
}
#endif

#endif // MESH_CONTROL_H
