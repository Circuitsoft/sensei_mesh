/* Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */
#ifndef PCA10040_H
#define PCA10040_H

#ifdef __cplusplus
extern "C" {
#endif

// LEDs definitions for PCA10040
#define LEDS_NUMBER 4

#define LED_START 17
#define LED_1 17
#define LED_2 18
#define LED_3 19
#define LED_4 20
#define LED_STOP 20

#define LEDS_LIST                                                              \
  { LED_1, LED_2, LED_3, LED_4 }

#define BSP_LED_0 LED_1
#define BSP_LED_1 LED_2
#define BSP_LED_2 LED_3
#define BSP_LED_3 LED_4

#define BSP_LED_0_MASK (1 << BSP_LED_0)
#define BSP_LED_1_MASK (1 << BSP_LED_1)
#define BSP_LED_2_MASK (1 << BSP_LED_2)
#define BSP_LED_3_MASK (1 << BSP_LED_3)

#define LEDS_MASK                                                              \
  (BSP_LED_0_MASK | BSP_LED_1_MASK | BSP_LED_2_MASK | BSP_LED_3_MASK)
/* all LEDs are lit when GPIO is low */
#define LEDS_INV_MASK LEDS_MASK

#define BUTTONS_NUMBER 4

#define BUTTON_START 13
#define BUTTON_1 13
#define BUTTON_2 14
#define BUTTON_3 15
#define BUTTON_4 16
#define BUTTON_STOP 16
#define BUTTON_PULL NRF_GPIO_PIN_PULLUP

#define BUTTONS_LIST                                                           \
  { BUTTON_1, BUTTON_2, BUTTON_3, BUTTON_4 }

#define BSP_BUTTON_0 BUTTON_1
#define BSP_BUTTON_1 BUTTON_2
#define BSP_BUTTON_2 BUTTON_3
#define BSP_BUTTON_3 BUTTON_4

#define BSP_BUTTON_0_MASK (1 << BSP_BUTTON_0)
#define BSP_BUTTON_1_MASK (1 << BSP_BUTTON_1)
#define BSP_BUTTON_2_MASK (1 << BSP_BUTTON_2)
#define BSP_BUTTON_3_MASK (1 << BSP_BUTTON_3)

#define BUTTONS_MASK 0x001E0000

#define RX_PIN_NUMBER 8
#define TX_PIN_NUMBER 6
#define CTS_PIN_NUMBER 7
#define RTS_PIN_NUMBER 5
#define HWFC true

// Low frequency clock source to be used by the SoftDevice & mesh

// If you have LFXTAL available
#define NRF_CLOCK_LFCLKSRC      {.source        = NRF_CLOCK_LF_SRC_XTAL,            \
                                 .rc_ctiv       = 0,                                \
                                 .rc_temp_ctiv  = 0,                                \
                                 .xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_20_PPM}

#ifdef __cplusplus
}
#endif

#endif // PCA10040_H
