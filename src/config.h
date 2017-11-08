#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "assert.h"
#include "fds.h"
#include "fstorage.h"
#include <stdbool.h>
#include <stdint.h>

const uint8_t DEFAULT_MESH_CHANNEL = 38;

typedef struct {
  uint8_t sensor_id{0};
  uint8_t serial_enabled{true};
  uint8_t mesh_channel{DEFAULT_MESH_CHANNEL};
  uint8_t sleep_enabled{true};
} __attribute__((packed)) app_config_t;

class Config_t {
private:
  // FDS needs this to be word-aligned
  alignas(int32_t) app_config_t backing_struct;

  bool loaded{false};

  const uint16_t config_file_id = 0xA0A0;    // Arbitrary
  const uint16_t config_record_key = 0x0640; // Arbitrary

  bool loadIfNotLoaded();
  ret_code_t write(uint8_t *data, uint8_t length);
  ret_code_t update(uint8_t *data, uint8_t length);
  bool read();
  bool save();

public:
  // Returns true on success
  bool Init();

  bool ToStruct(app_config_t *config_struct);
  uint8_t FromStruct(app_config_t *config_struct);

  inline uint8_t GetSensorID() {
    APP_ASSERT(loaded, "Config was not loaded (call `Init()`)");
    return backing_struct.sensor_id;
  }
  inline uint8_t GetMeshChannel() {
    APP_ASSERT(loaded, "Config was not loaded (call `Init()`)");
    return backing_struct.mesh_channel;
  }
  inline bool GetSerialEnabled() {
    APP_ASSERT(loaded, "Config was not loaded (call `Init()`)");
    return backing_struct.serial_enabled;
  }
  inline bool GetSleepEnabled() {
    APP_ASSERT(loaded, "Config was not loaded (call `Init()`)");
    return backing_struct.sleep_enabled;
  }

  void SetSensorID(uint8_t id);
  void SetMeshChannel(uint8_t channel);
  void SetSerialEnabled(bool enabled);
  void SetSleepEnabled(bool enabled);

  const uint8_t DEFAULT_MESH_CHANNEL = DEFAULT_MESH_CHANNEL;
};

extern Config_t Config;

#endif //__CONFIG_H__
