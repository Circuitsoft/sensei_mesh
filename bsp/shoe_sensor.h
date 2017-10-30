

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

#ifdef __cplusplus
}
#endif

#endif // SHOE_SENSOR_H
