#include "heartbeat.h"
#include "app_evt.h"
#include "assert.h"
#include "config.h"
#include "leds.h"
#include "mesh_control.h"
#include "mesh_packet.h"
#include "proximity.h"
#include "scheduler.h"
#include "transport_control.h"

static tc_tx_config_t m_tx_config;

void heartbeat_init(uint8_t channel) {
  m_tx_config.alt_access_address = false;
  m_tx_config.first_channel = channel;
  m_tx_config.channel_map = 1;
  m_tx_config.tx_power =
      static_cast<rbc_mesh_txpower_t>(mesh_control_get_hb_tx_power());
}

void send_heartbeat_packet(uint8_t sensor_id, uint32_t epoch_seconds,
                           uint16_t epoch_ms, uint16_t clock_version) {
  // Send out time sync packet
  log("send_heartbeat_packet()");
  mesh_packet_t *p_packet;
  if (mesh_packet_acquire(&p_packet)) {

    const uint8_t length = sizeof(heartbeat_ad_t);

    mesh_packet_set_local_addr(p_packet);

    p_packet->header.length =
        (BLE_GAP_ADDR_LEN + 1 /* adv_data_length */ + 1 /* adv_data_type */) +
        length;
    p_packet->header.type = BLE_PACKET_TYPE_ADV_NONCONN_IND;

    ble_ad_t *p_adv_data = (ble_ad_t *)&p_packet->payload[0];

    /* fill  adv data header fields */
    p_adv_data->adv_data_length = 1 + length;
    p_adv_data->adv_data_type =
        HEARTBEAT_ADV_DATA_TYPE; // Normal mesh packets are MESH_ADV_DATA_TYPE
                                 // (0x16)

    heartbeat_ad_t *p_heartbeat_ad = (heartbeat_ad_t *)&p_adv_data->data[0];
    p_heartbeat_ad->sensor_id = sensor_id;
    p_heartbeat_ad->epoch_seconds = epoch_seconds;
    p_heartbeat_ad->epoch_ms = epoch_ms;
    p_heartbeat_ad->clock_version = clock_version;

#ifdef CLOCK_MASTER
    m_tx_config.tx_power = RBC_MESH_TXPOWER_Pos4dBm;
#else
    m_tx_config.tx_power =
        static_cast<rbc_mesh_txpower_t>(mesh_control_get_hb_tx_power());
#endif

    if (tc_tx(p_packet, &m_tx_config) != NRF_SUCCESS) {
      TOGGLE_LED(LED_RED);
    }

    mesh_packet_ref_count_dec(p_packet);
  }
}

void received_heartbeat(heartbeat_ad_t *p_heartbeat_ad, uint8_t rssi) {
  logf("received_heartbeat(%d,%d)", p_heartbeat_ad->sensor_id, rssi);
  app_evt_t event;
  set_clock_time(p_heartbeat_ad->epoch_seconds, p_heartbeat_ad->epoch_ms,
                 CLOCK_SOURCE_RF, p_heartbeat_ad->clock_version);
  proximity_add_entry(p_heartbeat_ad->sensor_id, rssi);

  event.opcode = APP_EVT_OPCODE_HEARTBEAT;
  event.params.heartbeat.rssi = rssi;
  event.params.heartbeat.received_at = get_clock_time();
  event.params.heartbeat.received_at_ms = get_clock_ms();
  event.params.heartbeat.local_clock_version = get_clock_version();
  event.params.heartbeat.packet = *p_heartbeat_ad;
  app_event_send(&event);
}
