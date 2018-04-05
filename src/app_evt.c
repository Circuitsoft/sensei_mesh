#include "app_evt.h"
#include "serial_evt.h"
#include "serial_handler.h"
#include <string.h>

void app_event_send(app_evt_t *evt) {
  serial_evt_t serial_evt;
  serial_evt.opcode = SERIAL_EVT_OPCODE_APP_EVT;
  serial_evt.length = sizeof(serial_evt.params.app_evt) + 1;
  memcpy(&serial_evt.params.app_evt, evt, sizeof(serial_evt.params.app_evt));
  serial_handler_event_send(&serial_evt);
}
