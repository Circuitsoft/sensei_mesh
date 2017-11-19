#ifndef APP_CMD_H
#define APP_CMD_H

#include "config.h"
#include "toolchain.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
  APP_CMD_OPCODE_SET_TIME = 0x02,
  APP_CMD_OPCODE_SET_CONFIG = 0x03,
  APP_CMD_OPCODE_GET_CONFIG = 0x04,
  APP_CMD_OPCODE_IS_CONFIG_UPDATE_PENDING = 0x05,
} app_cmd_opcode_t;

typedef __packed_armcc struct {
  int32_t epoch;
  uint16_t ms;
} __packed_gcc app_cmd_params_set_clock_time_t;

typedef __packed_armcc struct {
  app_config_t config;
} __packed_gcc app_cmd_params_set_config_t;

typedef __packed_armcc struct {
  app_cmd_opcode_t opcode;
  __packed_armcc union {
    app_cmd_params_set_clock_time_t set_clock_time;
    app_cmd_params_set_config_t set_config;
  } __packed_gcc params;
} __packed_gcc app_cmd_t;

uint16_t app_cmd_handler(uint8_t *data, uint8_t len, uint8_t *response,
                         uint8_t *response_length);

#ifdef __cplusplus
}
#endif
#endif // APP_CMD_H
