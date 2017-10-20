#ifndef BOARD_LESSON_TRACKER_H
#define BOARD_LESSON_TRACKER_H

#ifdef __cplusplus
extern "C" {
#endif

#define MMA8541
#define JOSTLE_DETECT

// LEDs definitions for SENSEI_AREA_H
#define LEDS_NUMBER    1

#define LED_START      8
#define LED_1          8
#define LED_STOP       8

#define LEDS_ACTIVE_STATE 0

#define LEDS_INV_MASK  LEDS_MASK

#define LEDS_LIST { LED_1 }

#define BSP_LED_0      LED_1

#define BUTTONS_NUMBER 1

#define BUTTON_START   21
#define BUTTON_1       21
#define BUTTON_STOP    21
#define BUTTON_PULL    NRF_GPIO_PIN_PULLUP

#define BUTTONS_ACTIVE_STATE 0

#define BUTTONS_LIST { BUTTON_1 }

#define BSP_BUTTON_0   BUTTON_1

#define RX_PIN_NUMBER  3
#define TX_PIN_NUMBER  2
#define HWFC           false

#ifdef __cplusplus
}
#endif

#endif //BOARD_LESSON_TRACKER_H
