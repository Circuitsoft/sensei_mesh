#ifndef SHOE_SENSOR_H
#define SHOE_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

//#define MMA8541

// LEDs definitions for Shoe sensor v2
#define LEDS_NUMBER 0

#define BUTTONS_NUMBER 0

#define RX_PIN_NUMBER 3
#define TX_PIN_NUMBER 2
#define HWFC false

// Low frequency clock source to be used by the SoftDevice & mesh

// If you have LFXTAL available
#define NRF_CLOCK_LFCLKSRC      {.source        = NRF_CLOCK_LF_SRC_XTAL,            \
                                 .rc_ctiv       = 0,                                \
                                 .rc_temp_ctiv  = 0,                                \
                                 .xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_20_PPM}

#ifdef __cplusplus
}
#endif

#endif // SHOE_SENSOR_H
