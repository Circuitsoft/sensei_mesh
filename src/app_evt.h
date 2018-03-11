#ifndef APP_EVT_H
#define APP_EVT_H

#include "heartbeat.h"
#include "toolchain.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { APP_EVT_OPCODE_HEARTBEAT = 0x01 } app_evt_opcode_t;

typedef __packed_armcc struct {
  uint8_t rssi;
  int32_t received_at;
  uint16_t received_at_ms;
  uint16_t local_clock_version;
  heartbeat_ad_t packet;
} __packed_gcc app_evt_params_heartbeat_t;

typedef __packed_armcc struct {
  app_evt_opcode_t opcode;
  __packed_armcc union {
    app_evt_params_heartbeat_t heartbeat;
  } __packed_gcc params;
} __packed_gcc app_evt_t;

void app_event_send(app_evt_t *evt);

#ifdef __cplusplus
}
#endif

#endif // APP_EVT_H
