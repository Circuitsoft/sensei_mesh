#ifndef PROXIMITY_H
#define PROXIMITY_H

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PROXIMITY_TRACKING_COUNT 5

void proximity_add_entry(uint8_t sensor_id, uint8_t rssi);

void proximity_values_reset();

// returns number of sensors we are returning rssi for
// count = length of sensor_ids and rssi_values arrays
uint8_t proximity_get_strongest_signals(uint8_t *sensor_ids,
                                        uint8_t *rssi_values,
                                        uint8_t output_array_size);

#ifdef __cplusplus
}
#endif

#endif // PROXIMITY_H
