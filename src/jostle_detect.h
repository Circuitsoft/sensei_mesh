#ifndef JOSTLE_DETECT_H
#define JOSTLE_DETECT_H

#include "boards.h"
#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

extern volatile uint32_t last_jostle;

void jostle_detect_init();
bool jostle_detect_get_flag();
void jostle_detect_clear_flag();
void jostle_detect_check();
#ifdef JOSTLE_WAKEUP
void jostle_detect_enable();
void jostle_detect_disable();
#endif // JOSTLE_WAKEUP

#ifdef __cplusplus
}
#endif

#endif // JOSTLE_DETECT_H
