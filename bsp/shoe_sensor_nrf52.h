#ifndef SHOE_SENSOR_H
#define SHOE_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

#define JOSTLE_DETECT
#define JOSTLE_AVERAGE_THRESH 0.03f

#define JOSTLE_WAKEUP
// Slope in acceleration in
// 7.81 milli-G per sample (15.62Hz)
#define JOSTLE_WAKEUP_SLOPE_THRESH 8

#define BMX055
#define IMU_INT1_GPIO 8
#define IMU_INT3_GPIO 6

#define I2C_SDA_GPIO 7
#define I2C_SCL_GPIO 5

// Battery ADC Config
#define BATTERY_SENSE_PIN NRF_SAADC_INPUT_VDD
#define BATTERY_SENSE_ADC_GAIN NRF_SAADC_GAIN1_5
#define BATTERY_SENSE_ADC_RESOLUTION NRF_SAADC_RESOLUTION_12BIT
// ADC scale =  GAIN/REFERENCE * 2^(RESOLUTION)
#define BATTERY_SENSE_ADC_SCALE ((1/5.0)/0.6*(1<<12))
// No voltage divider
#define BATTERY_SENSE_EXTERNAL_SCALE 1

#define BATTERY_MAX_VOLTAGE 3.3
#define BATTERY_MIN_VOLTAGE 2.1

// Disable BSP
#define LEDS_NUMBER 0
#define BUTTONS_NUMBER 0

#define STATUS_LED_PIN 4

#define EXTRA_GPIO_PIN 9

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
