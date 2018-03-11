#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include "toolchain.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HEARTBEAT_ADV_DATA_TYPE (0x48)

typedef __packed_armcc struct {
  uint8_t sensor_id;
  int32_t epoch_seconds;
  uint16_t epoch_ms;
  uint16_t clock_version;
} __packed_gcc heartbeat_ad_t;

void heartbeat_init(uint8_t channel);
void send_heartbeat_packet(uint8_t sensor_id, uint32_t epoch_seconds,
                           uint16_t current_ms, uint16_t clock_version);
void received_heartbeat(heartbeat_ad_t *p_heartbeat_ad, uint8_t rssi);

#ifdef __cplusplus
}
#endif

#endif // HEARTBEAT_H
