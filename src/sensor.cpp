
#include "sensor.h"
#include "assert.h"
#include "battery.h"
#include "boards.h"
#include "config.h"
#include "handles.h"
#include "jostle_detect.h"
#include "rbc_mesh.h"
#include "scheduler.h"
#include "shoe_accel.h"
#include <app_error.h>
#include <string.h>

static sensor_value_t m_value;

void sensor_init() {
  log("sensor_init");
#ifdef ACCEL_ADXL337
  shoe_accel_init();
#endif

#ifdef JOSTLE_DETECT
  log("jostle detect init");
  jostle_detect_init();
#endif

  uint32_t error_code;
  error_code = rbc_mesh_value_enable(SENSOR_HANDLE(Config.GetSensorID()));
  APP_ERROR_CHECK(error_code);
}

void sensor_warmup_event() {
#ifdef ACCEL_ADXL337
  enable_shoe_accel();
#endif
}

void gather_sensor_data() {
  float voltage;
  memset(&m_value, 0, sizeof(sensor_value_t));
  m_value.valid_time = get_clock_time();
  voltage = get_battery_voltage();
  // Convert to percent
  m_value.battery = (voltage - BATTERY_MAX_VOLTAGE) / (BATTERY_MAX_VOLTAGE - BATTERY_MIN_VOLTAGE);

#ifdef JOSTLE_DETECT
  if (jostle_detect_get_flag()) {
    m_value.status |= STATUS_FLAG_JOSTLE_DETECTED;
    jostle_detect_clear_flag();
  }
#endif

#ifdef ACCEL_ADXL337
  read_shoe_accel(&m_value.accel_x, &m_value.accel_y, &m_value.accel_z);
#endif

  proximity_get_strongest_signals(m_value.proximity_ids, m_value.proximity_rssi,
                                  MAX_PROXIMITY_TRACKING_COUNT);
  proximity_values_reset();

#ifdef ACCEL_ADXL337
  disable_shoe_accel();
#endif
}

void report_sensor_data() {
  uint32_t error_code;

  if (Config.GetSensorID() > 0) {
    gather_sensor_data();
    error_code =
        rbc_mesh_value_set(SENSOR_HANDLE(Config.GetSensorID()),
                           (uint8_t *)&m_value, sizeof(sensor_value_t));
    APP_ERROR_CHECK(error_code);
  } else {
    // Would be nice to report this somewhere.
    log("WARNING: Not reporting sensor data; sensor ID not set");
  }
}
