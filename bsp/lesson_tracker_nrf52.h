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

// Battery ADC Config
#define BATTERY_SENSE_PIN NRF_SAADC_INPUT_AIN2
#define BATTERY_SENSE_ADC_GAIN NRF_SAADC_GAIN1_6
#define BATTERY_SENSE_ADC_RESOLUTION NRF_SAADC_RESOLUTION_12BIT
// ADC scale =  GAIN/REFERENCE * 2^(RESOLUTION)
#define BATTERY_SENSE_ADC_SCALE ((1/6.0)/0.6*(1<<12))
// Voltage divider.  R1 = 3.9 MOhm, R2 = 10 MOhm
#define BATTERY_SENSE_EXTERNAL_SCALE (10.0 / (3.9 + 10.0))
//#define BATTERY_SENSE_EXTERNAL_SCALE (1.0 / (1.0 + 1.0))

#define BATTERY_MAX_VOLTAGE 4.2
#define BATTERY_MIN_VOLTAGE 2.7

// Disable BSP
#define LEDS_NUMBER 0
#define BUTTONS_NUMBER 0

#define STATUS_LED_PIN 8

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
