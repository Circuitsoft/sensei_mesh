#ifndef RTC_H
#define RTC_H
#include "nrf_drv_rtc.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const uint8_t RTC_CC_VALUE;
extern volatile uint32_t rtc_walltime;

uint32_t rtc_value(void);
void rtc_handler(nrf_drv_rtc_int_type_t int_type);
uint32_t rtc_to_walltime(uint32_t rtc_value);
void rtc_init(void);

#ifdef __cplusplus
}
#endif
#endif