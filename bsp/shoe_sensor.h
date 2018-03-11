

#ifndef SHOE_SENSOR_H
#define SHOE_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

#define ACCEL_ADXL337
#define SIMBLEE

// LEDs definitions for Sensei shoe sensor board
#define LEDS_NUMBER    0
#define BUTTONS_NUMBER 0

#define RX_PIN_NUMBER  0
#define TX_PIN_NUMBER  1
//#define CTS_PIN_NUMBER 0xff
//#define RTS_PIN_NUMBER 0xff
#define HWFC           false

#define SDA_PIN_NUMBER 26
#define SCL_PIN_NUMBER 27

// Low frequency clock source to be used by the SoftDevice
#define NRF_CLOCK_LFCLKSRC      {.source        = NRF_CLOCK_LF_SRC_XTAL,            \
                                 .rc_ctiv       = 0,                                \
                                 .rc_temp_ctiv  = 0,                                \
                                 .xtal_accuracy = NRF_CLOCK_LF_XTAL_ACCURACY_75_PPM}



#ifdef __cplusplus
}
#endif

#endif // SHOE_SENSOR_H
