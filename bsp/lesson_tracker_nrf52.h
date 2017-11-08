#ifndef BOARD_LESSON_TRACKER_H
#define BOARD_LESSON_TRACKER_H

#ifdef __cplusplus
extern "C" {
#endif

#define BMX055
#define IMU_INT1_GPIO 20
#define IMU_INT3_GPIO 18

#define I2C_SDA_GPIO 7
#define I2C_SCL_GPIO 5

// LEDs definitions for Lesson Tracker v2
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

#endif //BOARD_LESSON_TRACKER_H
