#ifndef MESH_CONTROL_H
#define MESH_CONTROL_H

#include <stdint.h>
#include "toolchain.h"

// Default wake interval is 10 seconds
#define DEFAULT_WAKE_INTERVAL (10)
#define DEFAULT_RADIO_WINDOW_DURATION (900)

typedef __packed_armcc struct
{
  uint16_t wake_interval;
  uint8_t hb_tx_power;
  uint8_t enable_ble;  // Not used yet...
  uint16_t radio_window_duration;
} __packed_gcc mesh_control_t;


void mesh_control_init();

// Wake interval controls the cycle of waking and sleeping
// current_epoch % wake_interval == 0 indicates the start of a wake period
uint16_t mesh_control_get_wake_interval();

// Heartbeat tx power controls the tx power level of sent heartbeat packets
uint8_t mesh_control_get_hb_tx_power();

void mesh_control_update_config(mesh_control_t *new_config);

#endif // MESH_CONTROL_H
