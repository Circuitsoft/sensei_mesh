#ifndef __SCHEDULER_H_
#define __SCHEDULER_H_

#include <stdint.h>
#include "toolchain.h"
#include <stdbool.h>

typedef enum {
  CLOCK_SOURCE_UNSET,
  CLOCK_SOURCE_RF,
  CLOCK_SOURCE_SERIAL,
} clock_source_t;

typedef enum {
  SCHEDULER_STATE_STOPPED,
  SCHEDULER_STATE_SLEEP,
  SCHEDULER_STATE_BEFORE_HB,
  SCHEDULER_STATE_AFTER_HB,
  SCHEDULER_STATE_REPORTING,
} scheduler_state_t;

#define MAX_EXPECTED_CLOCK_SKEW_MS (10)
#define HEARTBEAT_WINDOW_MS (40)
#define TOTAL_RADIO_WINDOW_MS (900)

void scheduler_init();

void start_clock(uint16_t start_delay);

// If the source is CLOCK_SOURCE_SERIAL, the clock time will be updated unconditionally
// If the source is CLOCK_SOURCE_RF, the version will be checked, and if this is
// a newer version, then the clock will be updated.
void set_clock_time(int32_t epoch, uint16_t ms, clock_source_t clock_source, int16_t clock_version);

// Returns unix epoch
int32_t get_clock_time();

// Seconds since the sensor started
int32_t get_uptime();

// Returns true if the clock has been synchronized within the last hour
bool clock_is_synchronized();

#endif //__SCHEDULER_H_
