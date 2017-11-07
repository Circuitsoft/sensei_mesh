#include "config.h"
#include "assert.h"
#include "fds.h"
#include "fstorage.h"
#include "leds.h"
#include "nrf_error.h"
#include "power_manage.h"
#include "section_vars.h"
#include <string.h>

// TODO: Does this work on SDK8? (May need to port the pstorage code into this
// module)

volatile bool gc_complete;
volatile bool write_complete;
volatile bool init_complete;

static void fds_event_handler(fds_evt_t const *const p_fds_evt) {
  APP_ASSERT_EQUAL(p_fds_evt->result, FDS_SUCCESS,
                   "Received failed fds library callback");
  log("fds_event_handler()");
  switch (p_fds_evt->id) {
  case FDS_EVT_INIT:
    init_complete = true;
    if (p_fds_evt->result != FDS_SUCCESS) {
      log("initialization failed");
    } else {
      log("fds_initialized");
    }
    break;
  case FDS_EVT_WRITE:
    write_complete = true;
    log("write complete");
    break;
  case FDS_EVT_GC:
    gc_complete = true;
    log("gc complete");
    break;
  default:
    break;
  }
}

ret_code_t Config_t::write(uint8_t *data, uint8_t length) {
  fds_record_t record;
  fds_record_desc_t record_desc;
  fds_record_chunk_t record_chunk;

  APP_ASSERT(
      length * 4 < (FDS_VIRTUAL_PAGE_SIZE * 4) - 14,
      "Data length must not exceed FDS_VIRTUAL_PAGE_SIZE words minus 14 bytes");

  logf("Config length: %d", length);

  // Set up data.
  record_chunk.p_data = data;
  record_chunk.length_words = length;

  // Set up record.
  record.file_id = config_file_id;
  record.key = config_record_key;
  record.data.p_chunks = &record_chunk;
  record.data.num_chunks = 1;

  // fds_record_write runs asynchronously. Buffer above needs to stay in memory
  // until the write is complete (we get a notification via the event handler)
  write_complete = false;
  ret_code_t ret = fds_record_write(&record_desc, &record);
  if (ret == FDS_ERR_NO_SPACE_IN_FLASH) {
    log("Performing a flash garbage collection operation");
    // We're all out of space; run garbage collection
    // (this may take a while)
    gc_complete = false;
    ret_code_t ret = fds_gc();
    APP_ASSERT_EQUAL(ret, FDS_SUCCESS,
                     "Garbage collection queue request failed");

    // fds_gc() runs asynchronously, and can be fairly slow.
    // Since we know our data isn't going to change or move around, we
    // can just queue up the write for completion after the GC.
    //
    // WARNING: These assumptions shouldn't be taken for granted if this code is
    // copied for use elsewhere
    //
    // NOTE: Untested. From my one read of the docs, I think this will work?
    // If this doesn't work, the easy fix is to block until gc_complete is set
    while (!gc_complete)
      power_manage();

    ret = fds_record_write(&record_desc, &record);
  }

  APP_ASSERT_EQUAL(ret, FDS_SUCCESS, "Write queue request failed");
  if (ret != FDS_SUCCESS) {
    return ret;
  }
  return NRF_SUCCESS;
}

bool Config_t::read() {
  fds_flash_record_t flash_record;
  fds_record_desc_t record_desc;

  // Important, make sure you zero init the ftok token
  fds_find_token_t ftok = {0};
  ret_code_t err;

  // We currently make the assumption that our file_id and record_key uniquely
  // identify a single record. There are no guarantees behind this assumption.
  if (fds_record_find(config_file_id, config_record_key, &record_desc, &ftok) ==
      FDS_SUCCESS) {
    err = fds_record_open(&record_desc, &flash_record);
    APP_ASSERT_EQUAL(err, FDS_SUCCESS, "Failed to open record");

    // Access the record through the flash_record structure.
    auto data = reinterpret_cast<const app_config_t *>(flash_record.p_data);
    backing_struct.sensor_id = data->sensor_id;
    backing_struct.mesh_channel = data->mesh_channel;
    backing_struct.serial_enabled = data->serial_enabled;
    backing_struct.sleep_enabled = data->sleep_enabled;

    // Close the record when done.
    err = fds_record_close(&record_desc);
    APP_ASSERT_EQUAL(err, FDS_SUCCESS, "Failed to close record");
    return true;
  }

  log("No config records found");

  // Didn't find any records
  return false;
}

ret_code_t Config_t::update(uint8_t *data, uint8_t length) {
  log("update()");
  fds_record_t record;
  fds_record_desc_t record_desc;
  fds_record_chunk_t record_chunk;
  fds_flash_record_t flash_record;
  ret_code_t err;

  APP_ASSERT(
      length * 4 < (FDS_VIRTUAL_PAGE_SIZE * 4) - 14,
      "Data length must not exceed FDS_VIRTUAL_PAGE_SIZE words minus 14 bytes");

  // Set up data.
  record_chunk.p_data = data;
  record_chunk.length_words = length;

  record.data.p_chunks = &record_chunk;
  record.data.num_chunks = 1;

  // Important, make sure you zero init the ftok token
  fds_find_token_t ftok = {0};

  // Find current record (assumption: there are no duplicates)
  if (fds_record_find(config_file_id, config_record_key, &record_desc, &ftok) ==
      FDS_SUCCESS) {

    // fds_record_update runs asynchronously. Buffer above needs to stay in
    // memory until the write is complete (we get a notification via the event
    // handler)
    write_complete = false;
    err = fds_record_update(&record_desc, &record);
    if (err == FDS_ERR_NO_SPACE_IN_FLASH) {
      // We're all out of space; run garbage collection
      // (this may take a while)
      gc_complete = false;
      err = fds_gc();
      APP_ASSERT_EQUAL(err, FDS_SUCCESS,
                       "Garbage collection queue request failed");

      // fds_gc() runs asynchronously, and can be fairly slow.
      // Since we know our data isn't going to change or move around, we
      // can just queue up the write for completion after the GC.
      //
      // WARNING: These assumptions shouldn't be taken for granted if this code
      // is
      // copied for use elsewhere

      err = fds_record_update(&record_desc, &record);
      if (err != FDS_SUCCESS) {
        return err;
      }
    }
  }

  APP_ASSERT_EQUAL(err, FDS_SUCCESS, "Update queue request failed");
  return NRF_SUCCESS;
}

// Loads representation from flash into RAM
bool Config_t::loadIfNotLoaded() {
  if (loaded) {
    return true;
  }

  if (!read()) {
    // No settings in flash. Write some.
    log("Writing blank settings");
    write(reinterpret_cast<uint8_t *>(&backing_struct),
          sizeof(app_config_t) / 4);
  }

  loaded = true;
  return loaded;
}

bool Config_t::save() {
  log("save()");
  // Since our backing struct doesn't move around, we can just queue up this
  // update (without waiting for the callback to fire)
  auto ret = update(reinterpret_cast<uint8_t *>(&backing_struct),
                    sizeof(app_config_t) / 4);
  APP_ASSERT_EQUAL(ret, NRF_SUCCESS, "Config.save() unsuccessful");
  return ret == NRF_SUCCESS;
}

bool Config_t::Init() {
  loaded = false;
  init_complete = false;

  APP_ASSERT(config_record_key != 0x0000,
             "record_key 0x0000 is system reserved");
  APP_ASSERT(config_record_key < 0xBFFF, "max record_key is 0xBFFF");
  APP_ASSERT(config_file_id < 0xBFFF, "max file_id is 0xBFFF");

  ret_code_t ret = fds_register(fds_event_handler);
  if (ret != FDS_SUCCESS) {
    log("fds_register() failed");
    return false;
  }
  ret = fds_init();

  if (ret != NRF_SUCCESS) {
    log("fds_init() failed");
    return false;
  }

  while (!init_complete)
    power_manage();

  // Load on initialization
  loadIfNotLoaded();
  return true;
}

bool Config_t::ToStruct(app_config_t *config_struct) {
  config_struct->sensor_id = GetSensorID();
  config_struct->mesh_channel = GetMeshChannel();
  config_struct->serial_enabled = GetSerialEnabled();
  config_struct->sleep_enabled = GetSleepEnabled();
}

uint8_t Config_t::FromStruct(app_config_t *config_struct) {
  APP_ASSERT(loaded, "Config was not loaded (call `Init()`)");
  backing_struct.sensor_id = config_struct->sensor_id;
  backing_struct.mesh_channel = config_struct->mesh_channel;
  backing_struct.serial_enabled = config_struct->serial_enabled;
  backing_struct.sleep_enabled = config_struct->sleep_enabled;
  save();
}

// Getters operate entirely out of RAM
uint8_t Config_t::GetSensorID() {
  APP_ASSERT(loaded, "Config was not loaded (call `Init()`)");
  return backing_struct.sensor_id;
}
uint8_t Config_t::GetMeshChannel() {
  APP_ASSERT(loaded, "Config was not loaded (call `Init()`)");
  return backing_struct.mesh_channel;
}
bool Config_t::GetSerialEnabled() {
  APP_ASSERT(loaded, "Config was not loaded (call `Init()`)");
  return static_cast<bool>(backing_struct.serial_enabled);
}
bool Config_t::GetSleepEnabled() {
  APP_ASSERT(loaded, "Config was not loaded (call `Init()`)");
  return static_cast<bool>(backing_struct.sleep_enabled);
}

// Setters persist values immediately and update RAM representation
void Config_t::SetSensorID(uint8_t id) {
  APP_ASSERT(loaded, "Config was not loaded (call `Init()`)");
  if (backing_struct.sensor_id == id) {
    return;
  }
  backing_struct.sensor_id = id;
  save();
}
void Config_t::SetMeshChannel(uint8_t channel) {
  APP_ASSERT(loaded, "Config was not loaded (call `Init()`)");
  if (backing_struct.mesh_channel == channel) {
    return;
  }
  backing_struct.mesh_channel = channel;
  save();
}
void Config_t::SetSerialEnabled(bool enabled) {
  APP_ASSERT(loaded, "Config was not loaded (call `Init()`)");
  if (static_cast<bool>(backing_struct.serial_enabled) == enabled) {
    return;
  }
  backing_struct.serial_enabled = enabled;
  save();
}
void Config_t::SetSleepEnabled(bool enabled) {
  APP_ASSERT(loaded, "Config was not loaded (call `Init()`)");
  if (static_cast<bool>(backing_struct.sleep_enabled) == enabled) {
    return;
  }
  backing_struct.sleep_enabled = enabled;
  save();
}

Config_t Config;
