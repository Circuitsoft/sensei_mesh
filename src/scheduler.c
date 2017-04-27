
#include "scheduler.h"
#include "leds.h"
#include "sensor.h"
#include "app_timer.h"
#include "config.h"
#include "rand.h"
#include "heartbeat.h"
#include "sensor.h"
#include "rbc_mesh.h"
#include "handles.h"

//  Timer settings
#define APP_TIMER_PRESCALER             15   // divisor value - 1
#define TICKS_PER_SECOND                2048 // f / (APP_TIMER_PRESCALER+1)
#define TICKS_PER_100ms                 205 // f / (APP_TIMER_PRESCALER+1) / 10
#define APP_TIMER_MAX_TIMERS            3
#define APP_TIMER_OP_QUEUE_SIZE         4

static int32_t m_boot_time;
static int32_t m_last_sync;
static int32_t m_current_time;
static int16_t m_clock_version;
static app_timer_id_t m_clock_sync_timer_ID;
static app_timer_id_t m_offset_timer_ID;
static app_timer_id_t m_periodic_timer_ID;
static scheduler_state_t m_scheduler_state;
static bool m_sleep_enabled = true;
static prng_t m_rand;
static uint32_t m_clock_second_start_counter_value;

#define MS_TO_TICKS(MS) ((TICKS_PER_100ms * (MS)) / 100)
#define TICKS_TO_MS(TICKS) (100 * (TICKS) / TICKS_PER_100ms)

static void offset_timer_cb(void * p_context);
static void delay_to_heartbeat();

#define DEBUG_REGISTER_SIZE (16)
static uint8_t debug_counter;
static uint8_t debug_register[DEBUG_REGISTER_SIZE];

static void add_value_to_debug_register(uint8_t value) {
  debug_register[debug_counter % DEBUG_REGISTER_SIZE] = value;
  debug_counter++;
}
static void report_debug_register() {
  rbc_mesh_value_set(DEBUG_REGISTER_HANDLE, debug_register, DEBUG_REGISTER_SIZE);
}

static void periodic_timer_cb(void * p_context)
{
  m_current_time += 1;
  DBG_TICK_PIN(6);


  if (1 /*m_current_time % 10 == 0*/) {
    //report_debug_register();
    //led_config(LED_GREEN, 1);
    app_timer_cnt_get(&m_clock_second_start_counter_value);
    m_scheduler_state = SCHEDULER_STATE_BEFORE_HB;
    rbc_mesh_start();

    delay_to_heartbeat();
  }
}

static void delay_to_heartbeat() {
  uint16_t random_tx_delay = ((rand_prng_get(&m_rand) & 0x3ff) * HEARTBEAT_WINDOW_MS) / 0x3ff;
  int32_t delay_ticks = MS_TO_TICKS(MAX_EXPECTED_CLOCK_SKEW_MS + random_tx_delay);

  // This gives some sensors time to come online before data is collected.
  sensor_warmup_event();

  if (app_timer_start(m_offset_timer_ID, delay_ticks, NULL) != NRF_SUCCESS) {
    toggle_led(LED_RED);
  }
}

static void do_heartbeat() {
  //led_config(LED_GREEN, 0);
  uint32_t current_counter;
  app_timer_cnt_get(&current_counter);
  // Modulo wraparound makes this ok
  uint32_t elapsed_ticks_since_second_start = current_counter - m_clock_second_start_counter_value;
  send_heartbeat_packet(get_sensor_id(), m_current_time, TICKS_TO_MS(elapsed_ticks_since_second_start), m_clock_version);
}

static void delay_to_reporting() {
  // We do a fixed delay here, even though the last delay (delay_to_heartbeat)
  // was a random interval, as we want the reporting to start with some randomness
  // as well, to avoid all nodes starting the mesh sync at the same time
  int32_t delay_ticks = MS_TO_TICKS(HEARTBEAT_WINDOW_MS);
  app_timer_start(m_offset_timer_ID, delay_ticks, NULL);
}

static void do_reporting() {
  //led_config(LED_GREEN, 1);
  report_sensor_data();
}

static void delay_to_sleep() {
  uint32_t current_counter;
  app_timer_cnt_get(&current_counter);
  uint32_t elapsed_ticks_since_second_start = current_counter - m_clock_second_start_counter_value;
  int32_t delay_ticks = MS_TO_TICKS(TOTAL_RADIO_WINDOW_MS) - elapsed_ticks_since_second_start;
  if (delay_ticks > 5) {
    app_timer_start(m_offset_timer_ID, delay_ticks, NULL);
  } else {
    offset_timer_cb(NULL);
  }
}

static void do_sleep() {
  rbc_mesh_stop();
  //led_config(LED_GREEN, 0);
}

static void offset_timer_cb(void * p_context) {
  //app_timer_stop(m_offset_timer_ID);

  switch (m_scheduler_state) {
    case SCHEDULER_STATE_BEFORE_HB:
      do_heartbeat();
      m_scheduler_state = SCHEDULER_STATE_AFTER_HB;
      delay_to_reporting();
      break;
    case SCHEDULER_STATE_AFTER_HB:
      do_reporting();
      m_scheduler_state = SCHEDULER_STATE_REPORTING;
      delay_to_sleep();
      break;
    case SCHEDULER_STATE_REPORTING:
      if (m_sleep_enabled && clock_is_synchronized()) {
        do_sleep();
      }
      m_scheduler_state = SCHEDULER_STATE_SLEEP;
      // periodic timer will wake us next time
      break;
  default:
    toggle_led(LED_RED);
    break;
  }
}

static void start_periodic_timer() {
  app_timer_start(m_periodic_timer_ID, TICKS_PER_SECOND, NULL);
}

static void clock_sync_cb(void * p_context) {
  m_current_time += 1;
  start_periodic_timer();
}

void start_clock(uint16_t start_delay) {
  int32_t start_delay_ticks = (start_delay * (TICKS_PER_SECOND/1000.0));
  if (start_delay_ticks > 5) {
    app_timer_start(m_clock_sync_timer_ID, start_delay_ticks, NULL);
  } else {
    start_periodic_timer();
  }
}

void scheduler_init(bool sleep_enabled) {
  m_sleep_enabled = sleep_enabled;
  rand_prng_seed(&m_rand);
  m_scheduler_state = SCHEDULER_STATE_STOPPED;
  APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_MAX_TIMERS, APP_TIMER_OP_QUEUE_SIZE, false);
  app_timer_create(&m_periodic_timer_ID, APP_TIMER_MODE_REPEATED, periodic_timer_cb);
  app_timer_create(&m_clock_sync_timer_ID, APP_TIMER_MODE_SINGLE_SHOT, clock_sync_cb);
  app_timer_create(&m_offset_timer_ID, APP_TIMER_MODE_SINGLE_SHOT, offset_timer_cb);
}

void set_clock_time(int32_t epoch, uint16_t ms, clock_source_t clock_source, int16_t clock_version) {

  if (clock_source == CLOCK_SOURCE_RF) {
    // modulo math to handle wraparound
    uint16_t version_delta = clock_version - m_clock_version;
    if (version_delta > 0 && version_delta < 0xff00) {
      m_clock_version = clock_version;
    } else {
      // Older or same clock version
      return;
    }
  } else if (clock_source == CLOCK_SOURCE_SERIAL) {
    m_clock_version++;
  }
  TOGGLE_PIN(LED_BLUE + LED_START);
  m_boot_time += epoch - m_current_time;
  m_last_sync = m_current_time = epoch;
  uint16_t start_delay = (1000 - ms) % 1000;
  app_timer_stop(m_periodic_timer_ID);
  app_timer_stop(m_clock_sync_timer_ID);
  start_clock(start_delay);
}

int32_t get_clock_time() {
  return m_current_time;
}

int32_t get_uptime() {
  return m_current_time - m_boot_time;
}

bool clock_is_synchronized() {
  return m_last_sync > 0 && (m_current_time - m_last_sync) < (60 * 60);
}
